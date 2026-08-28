#include "quicktestengine.h"

#include <iostream>

#include <QBuffer>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QMouseEvent>
#include <QPromise>
#include <QQmlContext>
#include <QQuickItemGrabResult>
#include <QQuickWindow>
#include <QTimer>

namespace {
Q_LOGGING_CATEGORY(self, "quicktestengine") //, QtWarningMsg)
}

/*!
    Writes the QuickTestEngine::PathPart \a pathPart to the debug stream \a debug.
    Used for displaying path specifications in debugging output.
*/
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
    , _eventLogging{false}
    , _enabled{false}
    , _address{"127.0.0.1"}
    , _port{21129}
    , _webSocketServer{_registry}
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

/*!
    Finds a QML object matching the specified path.
    
    Initiates a search starting from the top-level QML windows, matching each PathPart
    sequentially to descend through the object hierarchy. Returns the matched object
    as a QVariant, or an invalid QVariant if no match is found.
    
    \internal
*/
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

bool QuickTestEngine::click(const QList<PathPart> &path, double relX, double relY)
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

    const QPointF localPos(item->width() * relX, item->height() * relY);
    const auto scenePos = item->mapToScene(localPos);
    const auto screenPos = window->mapToGlobal(scenePos.toPoint());

    qCDebug(self) << scenePos << screenPos;

    auto pressEvent = new QMouseEvent(QEvent::MouseButtonPress, scenePos, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, pressEvent);

    auto releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, scenePos, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, releaseEvent);

    return true;
}

bool QuickTestEngine::mousePress(const QList<PathPart> &path)
{
    auto variant = find(path);
    if (!variant.canConvert<QObject *>()) {
        qCCritical(self) << "mousePress: path did not resolve to an object:" << path;
        return false;
    }

    auto item = qobject_cast<QQuickItem *>(variant.value<QObject *>());
    if (!item) {
        qCCritical(self) << "mousePress: resolved object is not a QQuickItem:" << path;
        return false;
    }

    auto window = item->window();
    if (!window) {
        qCCritical(self) << "mousePress: item has no window:" << path;
        return false;
    }

    const auto center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const auto screenPos = window->mapToGlobal(center.toPoint());

    qCDebug(self) << "mousePress" << center << screenPos;

    auto pressEvent = new QMouseEvent(QEvent::MouseButtonPress, center, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, pressEvent);

    return true;
}

bool QuickTestEngine::mouseRelease(const QList<PathPart> &path)
{
    auto variant = find(path);
    if (!variant.canConvert<QObject *>()) {
        qCCritical(self) << "mouseRelease: path did not resolve to an object:" << path;
        return false;
    }

    auto item = qobject_cast<QQuickItem *>(variant.value<QObject *>());
    if (!item) {
        qCCritical(self) << "mouseRelease: resolved object is not a QQuickItem:" << path;
        return false;
    }

    auto window = item->window();
    if (!window) {
        qCCritical(self) << "mouseRelease: item has no window:" << path;
        return false;
    }

    const auto center = item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
    const auto screenPos = window->mapToGlobal(center.toPoint());

    qCDebug(self) << "mouseRelease" << center << screenPos;

    auto releaseEvent = new QMouseEvent(QEvent::MouseButtonRelease, center, screenPos, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QGuiApplication::postEvent(window, releaseEvent);

    return true;
}

QFuture<QByteArray> QuickTestEngine::takeScreenShotAsBase64(QQuickItem *item, const QString &format, int quality)
{
    qCInfo(self) << "taking screenshot (item, format, quality):" << item << format << quality;
    auto promise = std::make_shared<QPromise<QByteArray>>();
    promise->start();
    auto future = promise->future();

    if (!item) {
        qCWarning(self) << "item invalid!";
        promise->finish();
        return future;
    }

    auto grabResult = item->grabToImage();
    QObject::connect(grabResult.get(), &QQuickItemGrabResult::ready, item, [grabResult, promise, format, quality]() {
        QByteArray data;
        QBuffer buffer{&data};
        buffer.open(QIODevice::WriteOnly);
        grabResult->image().save(&buffer, qUtf8Printable(format), quality);
        promise->addResult(data.toBase64());

        qCInfo(self) << "screenshot comlete";
        promise->finish();
    });

    return future;
}

bool QuickTestEngine::eventLogging() const
{
    return _eventLogging;
}

void QuickTestEngine::setEventLogging(bool newEventLogging)
{
    if (_eventLogging == newEventLogging)
        return;

    _eventLogging = newEventLogging;
    emit eventLoggingChanged();

    if (_eventLogging) {
        QGuiApplication::instance()->installEventFilter(this);
        _recording.start = QDateTime::currentDateTime();
        _recording.end = QDateTime();
        _recording.frames.clear();
    }

    else {
        QGuiApplication::instance()->removeEventFilter(this);
        _recording.end = QDateTime::currentDateTime();
    }
}

bool QuickTestEngine::enabled() const
{
    return _enabled;
}

void QuickTestEngine::setEnabled(bool newEnabled)
{
    if (_enabled == newEnabled)
        return;

    _enabled = newEnabled;
    emit enabledChanged();
}

QString QuickTestEngine::address() const
{
    return _address;
}

void QuickTestEngine::setAddress(const QString &newAddress)
{
    if (_address == newAddress)
        return;
    _address = newAddress;
    emit addressChanged();
}

int QuickTestEngine::port() const
{
    return _port;
}

void QuickTestEngine::setPort(int newPort)
{
    if (_port == newPort)
        return;
    _port = newPort;
    emit portChanged();
}

QuickTestEngine::RecordingFrame::Type event2recording(QEvent::Type type)
{
    switch (type) {
    case QEvent::TouchBegin:
    case QEvent::MouseButtonPress:
        return QuickTestEngine::RecordingFrame::Press;
    case QEvent::TouchEnd:
    case QEvent::MouseButtonRelease:
        return QuickTestEngine::RecordingFrame::Release;
    default:
        return QuickTestEngine::RecordingFrame::Unknown;
    }
}

QuickTestEngine::Path path(const QObject *object)
{
    QuickTestEngine::Path path;

    while (object) {
        QuickTestEngine::PathPart pathPart;

        QQmlContext *context = qmlContext(object);
        if (context)
            pathPart.id = context->nameForObject(object);

        auto metaObject = object->metaObject();
        if (metaObject)
            pathPart.typeName = metaObject->className();

        pathPart.objectName = object->objectName();

        // @TODO: this might not be easily done, maybe via attached property?
        // pathPart.index = ???

        // Properties cannot be clicked so no need/possiblity to handle those
    }

    return path;
}

bool QuickTestEngine::eventFilter(QObject *object, QEvent *event)
{
    if (qobject_cast<QQuickWindow *>(object))
        return QObject::eventFilter(object, event);

    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        qCDebug(self) << object << event->type();
        _recording.frames.append({
            QDateTime::currentDateTime(),
            event2recording(event->type()),
            path(object),
        });

    default:
        break;
    }

    return QObject::eventFilter(object, event);
}
