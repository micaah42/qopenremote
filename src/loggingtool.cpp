#include "loggingtool.h"

#include <QCoreApplication>
#include <iostream>

LoggingTool *LoggingTool::sLoggingTool = nullptr;

void LoggingTool::handleMessageStatic(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    sLoggingTool->handleMessage(type, context, message);
}

LoggingTool::LoggingTool(QObject *parent)
    : QObject{parent}
    , _useStdOut{false}
    , _limit{15000}
    , _resizeTo{10000}
    , _records{new LogRecordListModel{this}}
{
    qInstallMessageHandler(LoggingTool::handleMessageStatic);
    sLoggingTool = this;
}

void LoggingTool::handleMessage(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString line;
    QString category = QString::fromUtf8(context.category);
    auto now = QDateTime::currentDateTime();
    {
        QMutexLocker mutex{&_mutex};

        line = qFormatLogMessage(type, context, message);

        if (_useStdOut && (type == QtInfoMsg || type == QtDebugMsg))
            std::cout << qPrintable(line) << std::flush;
        else
            std::cerr << qPrintable(line) << std::flush;
    }

    QMetaObject::invokeMethod(
        this,
        [this, type, category, message, line, now]() {
            auto newRecord = new LogRecord(_records);
            newRecord->setType(type);
            newRecord->setTimestamp(now);
            newRecord->setCategory(category);
            newRecord->setMessage(message);
            newRecord->setLine(line);

            _records->append(newRecord);

            if (_records->length() >= _limit)
                _records->removeRows(0, _limit - _resizeTo);
        },
        Qt::QueuedConnection
    );
}

QString LoggingTool::format() const
{
    return _format;
}

void LoggingTool::setFormat(const QString &newFormat)
{
    if (_format == newFormat)
        return;
    _format = newFormat;
    emit formatChanged();

    qSetMessagePattern(_format);
}

QString LoggingTool::rules() const
{
    return _rules;
}

void LoggingTool::setRules(const QString &newRules)
{
    if (_rules == newRules)
        return;
    _rules = newRules;
    emit rulesChanged();
}

bool LoggingTool::useStdOut() const
{
    return _useStdOut;
}

void LoggingTool::setUseStdOut(bool newUseStdOut)
{
    if (_useStdOut == newUseStdOut)
        return;
    _useStdOut = newUseStdOut;
    emit useStdOutChanged();
}

LogRecordListModel *LoggingTool::records() const
{
    return _records;
}

int LoggingTool::limit() const
{
    return _limit;
}

void LoggingTool::setLimit(int newLimit)
{
    if (_limit == newLimit)
        return;
    _limit = newLimit;
    emit limitChanged();
}

int LoggingTool::resizeTo() const
{
    return _resizeTo;
}

void LoggingTool::setResizeTo(int newResizeTo)
{
    if (_resizeTo == newResizeTo)
        return;

    _resizeTo = newResizeTo;
    emit resizeToChanged();
}

LogRecordListModel::LogRecordListModel(QObject *parent)
    : ListModel<LogRecord *>{parent}
{}
