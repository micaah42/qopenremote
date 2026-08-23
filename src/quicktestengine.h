#ifndef QUICKTESTENGINE_H
#define QUICKTESTENGINE_H

#include <QDebug>
#include <QFuture>
#include <QObject>
#include <QQuickItem>

class QuickTestEngine : public QObject
{
    Q_OBJECT
public:
    struct PathPart
    {
        // Filters applied in recursive search
        QString id;
        QString typeName;
        QString objectName;

        // Fixed access on the previously found object
        int index = -1;
        QString propertyName;
    };

    explicit QuickTestEngine(QObject *parent = nullptr);


public slots:
    QByteArray takeScreenShotAsBase64(QQuickItem *item, const QString &format = "PNG", int quality = 90);
    static bool isMatching(QObject *object, const PathPart &pathPart);

    // Finds the object matching the given path, starting from the root object of the QML engine.
    // Typically you want to use one of the members per PathPart, but you can use multiple members if you want to be more specific.
    // Try to find a compromise between path specificity and path stability, as the path may break if the QML structure changes.
    QVariant find(const QList<PathPart> &path);

    // Like find but will wait for a creation event for at most "timeout" ms before failing (returning an invalid variant)
    QFuture<QVariant> findAwait(const QList<PathPart> &path, int timeout);

    // Finds the item matching the path and posts mouse press/release events at its center.
    bool click(const QList<PathPart> &path);

signals:
};

QDebug operator<<(QDebug debug, const QuickTestEngine::PathPart &pathPart);

#endif // QUICKTESTENGINE_H
