#ifndef LOGGINGTOOL_H
#define LOGGINGTOOL_H

#include <QMutex>
#include <QObject>
#include <QQmlEngine>

#include <listmodel.h>

#include "logrecord.h"

class LogRecordListModel : public ListModel<LogRecord *>
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit LogRecordListModel(QObject *parent);
};

class LoggingTool : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool useStdOut READ useStdOut WRITE setUseStdOut NOTIFY useStdOutChanged FINAL)
    Q_PROPERTY(QString format READ format WRITE setFormat NOTIFY formatChanged FINAL)
    Q_PROPERTY(QString rules READ rules WRITE setRules NOTIFY rulesChanged FINAL)

    Q_PROPERTY(LogRecordListModel *records READ records CONSTANT FINAL)
    Q_PROPERTY(int limit READ limit WRITE setLimit NOTIFY limitChanged FINAL)
    Q_PROPERTY(int resizeTo READ resizeTo WRITE setResizeTo NOTIFY resizeToChanged FINAL)

public:
    static void handleMessageStatic(QtMsgType type, const QMessageLogContext &context, const QString &message);
    explicit LoggingTool(QObject *parent = nullptr);

    QString format() const;
    void setFormat(const QString &newFormat);

    QString rules() const;
    void setRules(const QString &newRules);

    bool useStdOut() const;
    void setUseStdOut(bool newUseStdOut);

    LogRecordListModel *records() const;

    int limit() const;
    void setLimit(int newLimit);

    int resizeTo() const;
    void setResizeTo(int newResizeTo);

public slots:
    void handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &message);

signals:
    void formatChanged();
    void rulesChanged();
    void useStdOutChanged();
    void limitChanged();
    void resizeToChanged();

private:
    static LoggingTool *sLoggingTool;
    bool _useStdOut;
    QString _format;
    QString _rules;
    int _limit;
    int _resizeTo;

    QMutex _mutex;
    LogRecordListModel *_records = nullptr;
};

#endif // LOGGINGTOOL_H
