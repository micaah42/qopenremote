#ifndef QUICKTESTENGINE_H
#define QUICKTESTENGINE_H

#include <QObject>

class QuickTestEngine : public QObject
{
    Q_OBJECT
public:
    explicit QuickTestEngine(QObject *parent = nullptr);

signals:
};

#endif // QUICKTESTENGINE_H
