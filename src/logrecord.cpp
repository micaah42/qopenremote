#include "logrecord.h"

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

QDateTime LogRecord::timestamp() const
{
    return _timestamp;
}

void LogRecord::setTimestamp(const QDateTime &newTimestamp)
{
    if (_timestamp == newTimestamp)
        return;
    _timestamp = newTimestamp;
    emit timestampChanged();
}
