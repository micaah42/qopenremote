#ifndef DELETIONUTILS_H
#define DELETIONUTILS_H

#include <QDebug>
#include <QObject>
#include <QPointer>
#include <QTimer>

template<class T>
void qDeleteAll(const QList<T> &list)
{
    for (auto const &v : std::as_const(list))
        v->deleteLater();
}

template<class T>
void qDeleteAll1(const QList<T> &objects, int duration = 1000)
{
    // Use QPointer to avoid re-deleting objects that might already have been destroyed
    QList<QPointer<QObject>> saveObjects{objects.begin(), objects.end()};

    QTimer::singleShot(duration, [saveObjects]() {
        for (auto const &object : std::as_const(saveObjects))
            if (object)
                object->deleteLater();
            else
                qWarning() << __PRETTY_FUNCTION__ << "object already deleted!";
    });
}

template<class T>
void qDeleteAll1(const T &object, int duration = 1000)
{
    qDeleteAll1(QList{object}, duration);
}

#endif // DELETIONUTILS_H
