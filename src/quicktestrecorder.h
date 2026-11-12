#ifndef QUICKTESTRECORDER_H
#define QUICKTESTRECORDER_H

#include "qopenremote_global.h"

#include <QDateTime>
#include <QJsonObject>
#include <QObject>

struct QOPENREMOTE_EXPORT PathPart
{
    QString id;
    QString typeName;
    QString objectName;

    int index = -1;
    QString propertyName;

    Q_GADGET
};

struct QOPENREMOTE_EXPORT Path : public QList<PathPart>
{
    Q_GADGET
};

Q_DECLARE_METATYPE(Path)

struct QOPENREMOTE_EXPORT RecordingFrame
{
    enum Type { Unknown, Press, Release };
    Q_ENUM(Type);

    QDateTime time;
    Type action;
    Path path;

    Q_GADGET
};

struct QOPENREMOTE_EXPORT Recording
{
    QDateTime start;
    QDateTime end;
    QList<RecordingFrame> frames;

    Q_GADGET
};

class QOPENREMOTE_EXPORT QuickTestRecorder : public QObject
{
    Q_OBJECT

public:
    explicit QuickTestRecorder(QObject *parent = nullptr);

    bool eventLogging() const;
    void setEventLogging(bool eventLogging);

    const Recording &recording() const;
    bool saveRecording(const QString &filename = {});

signals:
    void eventLoggingChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QJsonObject recordingToJson() const;

    bool _eventLogging = false;
    Recording _recording;
};

#endif // QUICKTESTRECORDER_H