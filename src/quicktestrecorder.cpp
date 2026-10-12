#include "quicktestrecorder.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickWindow>

#include <enumutil.h>

namespace {
Q_LOGGING_CATEGORY(self, "quicktestrecorder", QtWarningMsg)
} // namespace

RecordingFrame::Type event2recording(QEvent::Type type)
{
    switch (type) {
    case QEvent::TouchBegin:
    case QEvent::MouseButtonPress:
        return RecordingFrame::Press;
    case QEvent::TouchEnd:
    case QEvent::MouseButtonRelease:
        return RecordingFrame::Release;
    default:
        return RecordingFrame::Unknown;
    }
}

Path path(const QQuickItem *item)
{
    Path path;

    while (item) {
        PathPart pathPart;

        if (auto *context = qmlContext(item))
            pathPart.id = context->nameForObject(item);

        if (auto *metaObject = item->metaObject())
            pathPart.typeName = metaObject->className();

        pathPart.objectName = item->objectName();

        if (item->parentItem())
            item = item->parentItem();
        else if (item->parent())
            item = qobject_cast<QQuickItem *>(item->parent());
        else
            item = nullptr;

        path.append(pathPart);
    }

    return path;
}

QuickTestRecorder::QuickTestRecorder(QObject *parent)
    : QObject{parent}
{}

bool QuickTestRecorder::eventLogging() const
{
    return _eventLogging;
}

void QuickTestRecorder::setEventLogging(bool eventLogging)
{
    if (_eventLogging == eventLogging)
        return;

    _eventLogging = eventLogging;
    emit eventLoggingChanged();

    if (_eventLogging) {
        QGuiApplication::instance()->installEventFilter(this);
        _recording.start = QDateTime::currentDateTime();
        _recording.end = QDateTime();
        _recording.frames.clear();
        qCInfo(self) << "event recording started at" << _recording.start;
    } else {
        QGuiApplication::instance()->removeEventFilter(this);
        _recording.end = QDateTime::currentDateTime();
        qCInfo(self) << "event recording stopped at" << _recording.end << "with" << _recording.frames.size() << "frames";
    }
}

const Recording &QuickTestRecorder::recording() const
{
    return _recording;
}

QJsonObject QuickTestRecorder::recordingToJson() const
{
    QJsonArray frames;

    for (const auto &recordingFrame : _recording.frames) {
        QJsonArray path;

        for (const auto &pathPart : recordingFrame.path) {
            path.append(
                QJsonObject{
                    {"id", pathPart.id},
                    {"typeName", pathPart.typeName},
                    {"objectName", pathPart.objectName},
                    {"index", pathPart.index},
                    {"propertyName", pathPart.propertyName},
                }
            );
        }

        frames.append(
            QJsonObject{
                {"timestamp", recordingFrame.time.toMSecsSinceEpoch()},
                {"action", enumValueToKey(recordingFrame.action)},
                {"path", path},
            }
        );
    }

    return QJsonObject{
        {"start", _recording.start.toMSecsSinceEpoch()},
        {"end", _recording.end.toMSecsSinceEpoch()},
        {"frames", frames},
    };
}

bool QuickTestRecorder::saveRecording(const QString &filename)
{
    QString targetFilename = filename;
    if (targetFilename.isEmpty())
        targetFilename = QString("recording_%1.json").arg(_recording.start.toString("yyyy-MM-dd-hh-mm-ss"));

    QFile file(targetFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(self) << "failed to open file for writing:" << targetFilename;
        return false;
    }

    file.write(QJsonDocument{recordingToJson()}.toJson());
    qCInfo(self) << "saved recording to" << targetFilename;
    return true;
}

bool QuickTestRecorder::eventFilter(QObject *object, QEvent *event)
{
    if (qobject_cast<QQuickWindow *>(object))
        return QObject::eventFilter(object, event);

    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchEnd:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
        qCDebug(self) << "observed input event" << event->type() << "on" << object;
        _recording.frames.append({
            QDateTime::currentDateTime(),
            event2recording(event->type()),
            path(qobject_cast<QQuickItem *>(object)),
        });
        qCDebug(self) << "recorded event; total frames:" << _recording.frames.size();
        break;
    default:
        break;
    }

    return QObject::eventFilter(object, event);
}