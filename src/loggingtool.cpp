#include "loggingtool.h"

#include <QCoreApplication>
#include <QThread>
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
    QMutexLocker mutex{&_mutex};

    auto line = qFormatLogMessage(type, context, message);

    if (_useStdOut && (type == QtInfoMsg || type == QtDebugMsg))
        std::cout << qPrintable(line) << std::flush;
    else
        std::cerr << qPrintable(line) << std::flush;

    auto newRecord = new LogRecord{_records};
    newRecord->setType(type);
    newRecord->setCategory(context.category);
    newRecord->setMessage(message);
    newRecord->setLine(line);
    newRecord->moveToThread(QCoreApplication::instance()->thread());
    _records->append(newRecord);

    if (_records->length() >= _limit)
        _records->removeRows(0, _limit - _resizeTo);
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

LogRecord::LogRecord(QObject *parent)
    : QObject{parent}
    , _type{QtDebugMsg}
{}

QtMsgType LogRecord::type() const
{
    return _type;
}

void LogRecord::setType(QtMsgType newType)
{
    if (_type == newType)
        return;
    _type = newType;
    emit typeChanged();
}

QString LogRecord::category() const
{
    return _category;
}

void LogRecord::setCategory(const QString &newCategory)
{
    if (_category == newCategory)
        return;
    _category = newCategory;
    emit categoryChanged();
}

QString LogRecord::message() const
{
    return _message;
}

void LogRecord::setMessage(const QString &newMessage)
{
    if (_message == newMessage)
        return;
    _message = newMessage;
    emit messageChanged();
}

QString LogRecord::line() const
{
    return _line;
}

void LogRecord::setLine(const QString &newLine)
{
    if (_line == newLine)
        return;
    _line = newLine;
    emit lineChanged();
}

LogRecordListModel::LogRecordListModel(QObject *parent)
    : ListModel<LogRecord *>{parent}
{}
