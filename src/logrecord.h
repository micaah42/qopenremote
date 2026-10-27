#ifndef LOGRECORD_H
#define LOGRECORD_H

#include <QObject>
#include <QQmlEngine>

class LogRecord : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QtMsgType type READ type WRITE setType NOTIFY typeChanged FINAL)
    Q_PROPERTY(QDateTime timestamp READ timestamp WRITE setTimestamp NOTIFY timestampChanged FINAL)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY categoryChanged FINAL)
    Q_PROPERTY(QString message READ message WRITE setMessage NOTIFY messageChanged FINAL)
    Q_PROPERTY(QString line READ line WRITE setLine NOTIFY lineChanged FINAL)

public:
    enum Type {
        Debug = QtDebugMsg,
        Info = QtInfoMsg,
        Warning = QtWarningMsg,
        Critical = QtCriticalMsg,
        Fatal = QtFatalMsg,
    };
    Q_ENUM(Type);

    explicit LogRecord(QObject *parent = nullptr);

    QtMsgType type() const;
    void setType(QtMsgType newType);

    QString category() const;
    void setCategory(const QString &newCategory);

    QString message() const;
    void setMessage(const QString &newMessage);

    QString line() const;
    void setLine(const QString &newLine);

    QDateTime timestamp() const;
    void setTimestamp(const QDateTime &newTimestamp);

signals:
    void typeChanged();
    void categoryChanged();
    void messageChanged();
    void lineChanged();

    void timestampChanged();

private:
    QtMsgType _type;
    QString _category;
    QString _message;
    QString _line;
    QDateTime _timestamp;
};

#endif // LOGRECORD_H
