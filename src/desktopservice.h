#pragma once

#include "qopenremote_global.h"

#include <QObject>
#include <QQmlEngine>
#include <QUrl>

class QOPENREMOTE_EXPORT DesktopService : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit DesktopService(QObject *parent = nullptr)
        : QObject(parent)
    {}

public slots:
    QString readFile(const QString &filePath);
    bool openUrl(const QUrl &url) const;
};
