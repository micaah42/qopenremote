#ifndef QUICKTESTENGINE_H
#define QUICKTESTENGINE_H

#include <QObject>
#include <QQuickItem>
#include <QDebug>

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

    // Finds the object matching the given path, starting from the root object of the QML engine.
    // Typically you want to use one of the members per PathPart, but you can use multiple members if you want to be more specific.
    // Try to find a compromise between path specificity and path stability, as the path may break if the QML structure changes.
    QVariant find(const QList<PathPart> &path);

public slots:
    static bool isMatching(QObject *object, const PathPart &pathPart);
    QByteArray takeScreenShotAsBase64(QQuickItem *item, const QString &format = "PNG", int quality = 90);

signals:
};

QDebug operator<<(QDebug debug, const QuickTestEngine::PathPart &pathPart);

#endif // QUICKTESTENGINE_H
