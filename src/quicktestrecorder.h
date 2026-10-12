#ifndef QUICKTESTRECORDER_H
#define QUICKTESTRECORDER_H

#include <QDateTime>
#include <QJsonObject>
#include <QObject>

struct PathPart {
  QString id;
  QString typeName;
  QString objectName;

  int index = -1;
  QString propertyName;

  Q_GADGET
};

struct Path : public QList<PathPart> {
  Q_GADGET
};

Q_DECLARE_METATYPE(Path)

struct RecordingFrame {
  enum Type { Unknown, Press, Release };
  Q_ENUM(Type);

  QDateTime time;
  Type action;
  Path path;

  Q_GADGET
};

struct Recording {
  QDateTime start;
  QDateTime end;
  QList<RecordingFrame> frames;

  Q_GADGET
};

class QuickTestRecorder : public QObject {
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