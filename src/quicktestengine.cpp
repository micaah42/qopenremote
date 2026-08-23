#include "quicktestengine.h"

#include <QBuffer>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPromise>
#include <QQmlContext>
#include <QQuickWindow>
#include <QTimer>

namespace {
Q_LOGGING_CATEGORY(self, "quicktestengine") //, QtWarningMsg)
}

QDebug operator<<(QDebug debug, const QuickTestEngine::PathPart &pathPart)
{
    QDebugStateSaver saver(debug);
    debug.nospace()                                  //
        << "PathPart(id=" << pathPart.id             //
        << ",typeName=" << pathPart.typeName         //
        << ",objectName=" << pathPart.objectName     //
        << ",propertyName=" << pathPart.propertyName //
        << ",index=" << pathPart.index               //
        << ')';
    return debug;
}

QuickTestEngine::QuickTestEngine(QObject *parent)
    : QObject{parent}
{}

bool forEachChild(QObject *object, const std::function<bool(QObject *)> &callback)
{
    if (!object) {
        return false;
    }

    // Handle QQuickItems and their childItems()
    if (auto quickItem = qobject_cast<QQuickItem *>(object)) {
        for (auto *childItem : quickItem->childItems()) {
            if (!callback(childItem))
                return true;

            if (forEachChild(childItem, callback))
                return true;
        }
    }

    // Handle regular QObjects and their data()
    else {
        for (auto *child : object->children()) {
            if (!callback(child))
                return true;

            if (forEachChild(child, callback))
                return true;
        }
    }

    return false;
}

QVariant findIndexedChild(QObject *object, int index)
{
    if (!object || index < 0)
        return {};

    const auto metaObject = object->metaObject();
    QList<QByteArray> methodNames;

    const auto inheritsType = [metaObject](const char *typeName) {
        for (auto current = metaObject; current; current = current->superClass()) {
            if (qstrcmp(current->className(), typeName) == 0)
                return true;
        }
        return false;
    };

    if (inheritsType("QQuickItemView") || inheritsType("QQuickListView") || inheritsType("QQuickGridView") || inheritsType("QQuickTableView"))
        methodNames.append("itemAtIndex(int)");
    else if (inheritsType("QQuickRepeater") || inheritsType("QQuickPathView") || inheritsType("QQuickContainer"))
        methodNames.append("itemAt(int)");
    else {
        methodNames.append("itemAt(int)");
        methodNames.append("itemAtIndex(int)");
    }

    for (const auto &methodName : methodNames) {
        const auto methodIndex = metaObject->indexOfMethod(methodName.constData());
        if (methodIndex < 0)
            continue;

        const auto method = metaObject->method(methodIndex);
        QVariant returnValue(method.returnMetaType(), static_cast<void *>(nullptr));
        QGenericReturnArgument returnArgument(method.typeName(), const_cast<void *>(returnValue.constData()));
        QGenericArgument indexArgument("int", &index);

        if (method.invoke(object, Qt::DirectConnection, returnArgument, indexArgument)) {
            if (returnValue.canConvert<QObject *>())
                return returnValue;
        }
    }

    const auto children = object->children();
    if (index >= children.size())
        return {};

    return QVariant::fromValue(children.at(index));
}

QVariant QuickTestEngine::find(const QList<PathPart> &path)
{
    auto mutablePath = path;

    if (path.length() == 0) {
        qCCritical(self) << "path may not be empty!";
        return {};
    }

    QObject *currentNode = nullptr;
    const auto windows = QGuiApplication::topLevelWindows();

    for (auto window : windows) {
        QQuickWindow *qquickWindow = qobject_cast<QQuickWindow *>(window);

        if (!qquickWindow)
            continue;

        if (isMatching(qquickWindow, path.first())) {
            currentNode = qquickWindow;
            break;
        }
    }

    if (!currentNode) {
        qCCritical(self) << "failed to find a window matching:" << path;
        return {};
    }

    mutablePath.removeFirst();

    while (!mutablePath.empty()) {
        auto pathPart = mutablePath.first();

        if (pathPart.index != -1) {
            auto selectedChild = findIndexedChild(currentNode, pathPart.index);
            if (selectedChild.isNull()) {
                qCCritical(self) << "failed to resolve indexed child:" << pathPart << "from" << currentNode;
                return {};
            }

            if (mutablePath.size() == 1)
                return selectedChild;

            mutablePath.removeFirst();
            currentNode = selectedChild.value<QObject *>();
            continue;
        }

        if (!pathPart.propertyName.isEmpty()) {
            auto propertyValue = currentNode->property(qUtf8Printable(pathPart.propertyName));

            if (propertyValue.canConvert<QObject *>())
                currentNode = propertyValue.value<QObject *>();
            else if (mutablePath.size() == 1)
                return propertyValue;
            else {
                qCCritical(self) << "selected value reached a non-object and cannot descend further!";
                return {};
            }
            mutablePath.removeFirst();
            continue;
        }

        bool foundChild = forEachChild(currentNode, [&](QObject *child) {
            if (isMatching(child, pathPart)) {
                mutablePath.removeFirst();
                currentNode = child;
                return false;
            }

            return true;
        });

        if (!foundChild)
            return {};
    }

    return QVariant::fromValue(currentNode);
}

bool QuickTestEngine::isMatching(QObject *object, const PathPart &pathPart)
{
    if (!object)
        return false;

    if (!pathPart.typeName.isEmpty() && pathPart.typeName != object->metaObject()->className())
        return false;

    if (!pathPart.objectName.isEmpty() && object->objectName() != pathPart.objectName)
        return false;

    // if (!pathPart.propertyName.isEmpty()) {
    // const auto propertyName = pathPart.propertyName.toUtf8();
    // const auto metaPropertyIndex = object->metaObject()->indexOfProperty(propertyName.constData());
    // const auto dynamicProps = object->dynamicPropertyNames();
    // const bool hasMatchingProperty = metaPropertyIndex >= 0 || std::any_of(dynamicProps.begin(), dynamicProps.end(), [propertyName](const QByteArray &candidate) {
    // return candidate == propertyName;
    // });

    // if (!hasMatchingProperty)
    // return false;
    // }

    QQmlContext *context = qmlContext(object);
    if (context)
        if (!pathPart.id.isEmpty() && context->nameForObject(object) != pathPart.id)
            return false;

    return true;
}

QFuture<QVariant> QuickTestEngine::findAwait(const QList<PathPart> &path, int timeout)
{
    auto promise = std::make_shared<QPromise<QVariant>>();
    promise->start();
    auto future = promise->future();

    auto elapsed = std::make_shared<QElapsedTimer>();
    elapsed->start();

    auto timer = std::make_shared<QTimer>();
    timer->setInterval(16);

    connect(timer.get(), &QTimer::timeout, this, [this, path, timeout, promise, elapsed, timer]() {
        auto result = find(path);
        if (!result.isValid() && !elapsed->hasExpired(timeout))
            return;

        promise->addResult(result);
        promise->finish();
        timer->stop();
    });
    timer->start();

    return future;
}

bool QuickTestEngine::click(const QList<PathPart> &path)
{
    auto variant = find(path);
    if (!variant.canConvert<QObject *>()) {
        qCCritical(self) << "click: path did not resolve to an object:" << path;
        return false;
    }

    auto item = qobject_cast<QQuickItem *>(variant.value<QObject *>());
    if (!item) {
        qCCritical(self) << "click: resolved object is not a QQuickItem:" << path;
        return false;
    }

    auto window = item->window();
    if (!window) {
        qCCritical(self) << "click: item has no window:" << path;
        return false;
    }

    const auto center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const auto screenPos = window->mapToGlobal(center.toPoint());

    qCDebug(self) << center << screenPos;

    auto pressEvent = new QMouseEvent(QEvent::MouseButtonPress, center, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, pressEvent);

    auto releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, center, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, releaseEvent);

    return true;
}

QByteArray QuickTestEngine::takeScreenShotAsBase64(QQuickItem *item, const QString &format, int quality)
{
    auto window = item->window();
    auto image = window->grabWindow();

    QByteArray data;

    {
        QBuffer buffer{&data};
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, qUtf8Printable(format), quality);
    }

    return data.toBase64();
}
