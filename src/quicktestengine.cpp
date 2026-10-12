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
Q_LOGGING_CATEGORY(self, "quicktestengine", QtWarningMsg)

Path pathFromVariantList(const QVariantList &variants) {
  Path path;
  path.reserve(variants.size());

  for (const auto &variant : variants) {
    const auto map = variant.toMap();
    PathPart pathPart;
    pathPart.id = map.value("id").toString();
    pathPart.typeName = map.value("typeName").toString();
    pathPart.objectName = map.value("objectName").toString();
    pathPart.propertyName = map.value("propertyName").toString();
    if (map.contains("index"))
      pathPart.index = map.value("index").toInt();
    path.append(pathPart);
  }

  return path;
}
} // namespace

/*!
    Writes the QuickTestEngine::PathPart \a pathPart to the debug stream \a
   debug. Used for displaying path specifications in debugging output.
*/
QDebug operator<<(QDebug debug, const PathPart &pathPart) {
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
    : QObject{parent}, _enabled{false}, _address{"127.0.0.1"}, _port{21129},
      _recorder{this}, _webSocketServer{_registry} {
  static const bool pathConverterRegistered =
      QMetaType::registerConverter<QVariantList, Path>(pathFromVariantList);
  Q_ASSERT(pathConverterRegistered);

  _registry.registerValue("quickTestEngine", this);
  connect(&_recorder, &QuickTestRecorder::eventLoggingChanged, this,
          &QuickTestEngine::eventLoggingChanged);
  _webSocketServer.open();
  qCInfo(self) << "initialized quick test engine with websocket server"
               << &_webSocketServer;
}

bool forEachChild(QObject *object,
                  const std::function<bool(QObject *)> &callback) {
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

  // @TODO: Research handle regular QObjects children
  // These children seem to contain pointers to freed children :(

  else {
    for (auto child : object->children()) {
      if (!callback(child))
        return true;

      if (forEachChild(child, callback))
        return true;
    }
  }

  return false;
}

QVariant findIndexedChild(QObject *object, int index) {
  if (!object || index < 0) {
    qCDebug(self) << "cannot resolve indexed child for" << object << "at index"
                  << index;
    return {};
  }

  const auto metaObject = object->metaObject();
  QList<QByteArray> methodNames;

  qCDebug(self) << "resolving indexed child for" << object << "at index"
                << index;

  const auto inheritsType = [metaObject](const char *typeName) {
    for (auto current = metaObject; current; current = current->superClass()) {
      if (qstrcmp(current->className(), typeName) == 0)
        return true;
    }
    return false;
  };

  if (inheritsType("QQuickItemView") || inheritsType("QQuickListView") ||
      inheritsType("QQuickGridView") || inheritsType("QQuickTableView"))
    methodNames.append("itemAtIndex(int)");
  else if (inheritsType("QQuickRepeater") || inheritsType("QQuickPathView") ||
           inheritsType("QQuickContainer"))
    methodNames.append("itemAt(int)");
  else {
    methodNames.append("itemAt(int)");
    methodNames.append("itemAtIndex(int)");
  }

  for (const auto &methodName : methodNames) {
    const auto methodIndex = metaObject->indexOfMethod(methodName.constData());
    if (methodIndex < 0) {
      qCDebug(self) << "indexed-child accessor is unavailable:" << methodName
                    << "on" << object;
      continue;
    }

    const auto method = metaObject->method(methodIndex);
    QVariant returnValue(method.returnMetaType(), static_cast<void *>(nullptr));
    QGenericReturnArgument returnArgument(
        method.typeName(), const_cast<void *>(returnValue.constData()));
    QGenericArgument indexArgument("int", &index);

    if (method.invoke(object, Qt::DirectConnection, returnArgument,
                      indexArgument)) {
      if (returnValue.canConvert<QObject *>()) {
        qCDebug(self) << "resolved indexed child using" << methodName << ':'
                      << returnValue;
        return QVariant{method.returnMetaType(), returnValue.constData()};
      }

      qCDebug(self) << "indexed-child accessor returned a non-object:"
                    << methodName << returnValue;
    } else {
      qCDebug(self) << "failed to invoke indexed-child accessor:" << methodName
                    << "on" << object;
    }
  }

  const auto children = object->children();
  if (index >= children.size()) {
    qCDebug(self) << "indexed child fallback is out of range:" << index << "of"
                  << children.size() << "on" << object;
    return {};
  }

  const auto child = children.at(index);
  qCDebug(self) << "resolved indexed child using QObject child fallback:"
                << child;
  return QVariant::fromValue(child);
}

/*!
    Finds a QML object matching the specified path.

    Initiates a search starting from the top-level QML windows, matching each
   PathPart sequentially to descend through the object hierarchy. Returns the
   matched object as a QVariant, or an invalid QVariant if no match is found.

    \internal
*/
QVariant QuickTestEngine::find(const Path &path) {
  auto mutablePath = path;

  qCDebug(self) << "finding path:" << path;

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

  qCDebug(self) << "matched root window:" << currentNode;
  mutablePath.removeFirst();

  while (!mutablePath.empty()) {
    auto pathPart = mutablePath.first();

    if (pathPart.index != -1) {
      auto selectedChild = findIndexedChild(currentNode, pathPart.index);
      if (selectedChild.isNull()) {
        qCCritical(self) << "failed to resolve indexed child:" << pathPart
                         << "from" << currentNode;
        return {};
      }

      qCDebug(self) << "resolved indexed child:" << pathPart << "to"
                    << selectedChild;

      if (mutablePath.size() == 1) {
        qCDebug(self) << "path resolved to indexed child:" << selectedChild;
        return selectedChild;
      }

      mutablePath.removeFirst();
      currentNode = selectedChild.value<QObject *>();
      continue;
    }

    if (!pathPart.propertyName.isEmpty()) {
      auto propertyValue =
          currentNode->property(qUtf8Printable(pathPart.propertyName));

      if (propertyValue.canConvert<QObject *>()) {
        currentNode = propertyValue.value<QObject *>();
        qCDebug(self) << "resolved object property:" << pathPart.propertyName
                      << "to" << currentNode;
      } else if (mutablePath.size() == 1) {
        qCDebug(self) << "path resolved to property:" << pathPart.propertyName
                      << propertyValue;
        return propertyValue;
      } else {
        qCCritical(self) << "selected value reached a non-object and cannot "
                            "descend further!";
        return {};
      }
      mutablePath.removeFirst();
      continue;
    }

    bool foundChild = forEachChild(currentNode, [&](QObject *child) {
      if (isMatching(child, pathPart)) {
        mutablePath.removeFirst();
        currentNode = child;
        qCDebug(self) << "matched path part:" << pathPart << "to"
                      << currentNode;
        return false;
      }

      return true;
    });

    if (!foundChild) {
      qCWarning(self) << "failed to find matching child:" << pathPart << "below"
                      << currentNode;
      return {};
    }
  }

  qCDebug(self) << "path resolved to:" << currentNode;
  return QVariant::fromValue(currentNode);
}

bool QuickTestEngine::isMatching(QObject *object, const PathPart &pathPart) {
  if (!object)
    return false;

  qCDebug(self) << "matching (object, pathPart)" << object << pathPart;

  if (!pathPart.typeName.isEmpty() &&
      pathPart.typeName != object->metaObject()->className())
    return false;

  if (!pathPart.objectName.isEmpty() &&
      object->objectName() != pathPart.objectName)
    return false;

  if (!pathPart.id.isEmpty()) {
    QQmlContext *context = qmlContext(object);
    if (context) {
      auto id = context->nameForObject(object);
      if (id != pathPart.id)
        return false;
    }

    else {
      return false;
    }
  }

  qCDebug(self) << "match found!";
  return true;
}

QFuture<QVariant> QuickTestEngine::findAwait(const Path &path, int timeout) {
  qCInfo(self) << "waiting for path with timeout" << timeout << "ms:" << path;
  auto promise = std::make_shared<QPromise<QVariant>>();
  promise->start();
  auto future = promise->future();

  auto elapsed = std::make_shared<QElapsedTimer>();
  elapsed->start();

  auto timer = std::make_shared<QTimer>();
  timer->setInterval(16);

  connect(timer.get(), &QTimer::timeout, this,
          [this, path, timeout, promise, elapsed, timer]() {
            auto result = find(path);
            if (!result.isValid() && !elapsed->hasExpired(timeout))
              return;

            qCInfo(self) << (result.isValid() ? "path found after"
                                              : "path lookup timed out after")
                         << elapsed->elapsed() << "ms:" << path;
            promise->addResult(result);
            promise->finish();
            timer->stop();
          });
  timer->start();

  return future;
}

bool QuickTestEngine::click(const Path &path) {
  return this->clickPosition(path, 0.5, 0.5);
}

bool QuickTestEngine::clickPosition(const Path &path, double relX,
                                    double relY) {
  qCInfo(self) << "posting click for path at relative position" << relX << relY
               << ':' << path;
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
  emit mousePressed(scenePos);
  qCDebug(self) << "posting click events at scene, screen:" << scenePos
                << screenPos;

  auto pressEvent =
      new QMouseEvent(QEvent::MouseButtonPress, scenePos, screenPos,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QGuiApplication::postEvent(window, pressEvent);

  QTimer::singleShot(
      75, Qt::PreciseTimer, this, [this, window, scenePos, screenPos]() {
        auto releaseEvent =
            new QMouseEvent(QEvent::MouseButtonRelease, scenePos, screenPos,
                            Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QGuiApplication::postEvent(window, releaseEvent);
        emit mouseReleased(scenePos);
      });

  return true;
}

bool QuickTestEngine::mousePress(const Path &path) {
  qCInfo(self) << "posting mouse press for path:" << path;
  auto variant = find(path);
  if (!variant.canConvert<QObject *>()) {
    qCCritical(self) << "mousePress: path did not resolve to an object:"
                     << path;
    return false;
  }

  auto item = qobject_cast<QQuickItem *>(variant.value<QObject *>());
  if (!item) {
    qCCritical(self) << "mousePress: resolved object is not a QQuickItem:"
                     << path;
    return false;
  }

  auto window = item->window();
  if (!window) {
    qCCritical(self) << "mousePress: item has no window:" << path;
    return false;
  }

  const auto scenePos =
      item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
  const auto screenPos = window->mapToGlobal(scenePos.toPoint());
  qCDebug(self) << "mousePress" << scenePos << screenPos;

  auto pressEvent =
      new QMouseEvent(QEvent::MouseButtonPress, scenePos, screenPos,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QGuiApplication::postEvent(window, pressEvent);
  emit mousePressed(scenePos);

  return true;
}

bool QuickTestEngine::mouseRelease(const Path &path) {
  qCInfo(self) << "posting mouse release for path:" << path;
  auto variant = find(path);
  if (!variant.canConvert<QObject *>()) {
    qCCritical(self) << "mouseRelease: path did not resolve to an object:"
                     << path;
    return false;
  }

  auto item = qobject_cast<QQuickItem *>(variant.value<QObject *>());
  if (!item) {
    qCCritical(self) << "mouseRelease: resolved object is not a QQuickItem:"
                     << path;
    return false;
  }

  auto window = item->window();
  if (!window) {
    qCCritical(self) << "mouseRelease: item has no window:" << path;
    return false;
  }

  const auto scenePos =
      item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
  const auto screenPos = window->mapToGlobal(scenePos.toPoint());
  qCDebug(self) << "mouseRelease" << scenePos << screenPos;

  auto releaseEvent =
      new QMouseEvent(QEvent::MouseButtonRelease, scenePos, screenPos,
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
  QGuiApplication::postEvent(window, releaseEvent);
  emit mouseReleased(scenePos);
  return true;
}

QFuture<QByteArray>
QuickTestEngine::takeScreenShotAsBase64(QQuickItem *item, const QString &format,
                                        int quality) {
  qCInfo(self) << "taking screenshot (item, format, quality):" << item << format
               << quality;
  auto promise = std::make_shared<QPromise<QByteArray>>();
  promise->start();
  auto future = promise->future();

  if (!item) {
    qCWarning(self) << "item invalid!";
    promise->finish();
    return future;
  }

  auto grabResult = item->grabToImage();
  QObject::connect(grabResult.get(), &QQuickItemGrabResult::ready, item,
                   [grabResult, promise, format, quality]() {
                     QByteArray data;
                     QBuffer buffer{&data};
                     buffer.open(QIODevice::WriteOnly);
                     grabResult->image().save(&buffer, qUtf8Printable(format),
                                              quality);
                     promise->addResult(data.toBase64());

                     qCInfo(self) << "screenshot comlete";
                     promise->finish();
                   });

  return future;
}

bool QuickTestEngine::eventLogging() const { return _recorder.eventLogging(); }

const Recording &QuickTestEngine::recording() const {
  return _recorder.recording();
}

bool QuickTestEngine::saveRecording(const QString &filename) {
  return _recorder.saveRecording(filename);
}

void QuickTestEngine::setEventLogging(bool newEventLogging) {
  _recorder.setEventLogging(newEventLogging);
}

bool QuickTestEngine::enabled() const { return _enabled; }

void QuickTestEngine::setEnabled(bool newEnabled) {
  if (_enabled == newEnabled)
    return;

  _enabled = newEnabled;
  qCInfo(self) << "quick test engine enabled:" << _enabled;
  emit enabledChanged();
}

QString QuickTestEngine::address() const { return _address; }

void QuickTestEngine::setAddress(const QString &newAddress) {
  if (_address == newAddress)
    return;
  _address = newAddress;
  qCInfo(self) << "quick test engine address changed to" << _address;
  emit addressChanged();
}

int QuickTestEngine::port() const { return _port; }

void QuickTestEngine::setPort(int newPort) {
  if (_port == newPort)
    return;
  _port = newPort;
  qCInfo(self) << "quick test engine port changed to" << _port;
  emit portChanged();
}
