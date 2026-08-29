#ifndef QUICKTESTENGINE_H
#define QUICKTESTENGINE_H

#include <QDebug>
#include <QFuture>
#include <QObject>
#include <QQuickItem>

#include <objectregistry2.h>
#include <websocketserver.h>

/*!
        \struct PathPart
        \ingroup testing
        \brief Represents a single step in a path to locate a QML object.

        PathPart defines criteria for matching a QML object during recursive search.
        Multiple members can be combined for more specific matching, though a balance
        should be maintained between specificity and stability to account for QML
        structure changes.

        \var PathPart::id
            The QML id attribute to match during the search. Used with QQmlContext::nameForObject().

        \var PathPart::typeName
            The C++ type name (class name) to match. Compared against metaObject()->className().

        \var PathPart::objectName
            The QObject::objectName property to match.

        \var PathPart::index
            The index of a child item when accessing indexed children (e.g., ListModel items).
            Default value is -1, indicating index-based access is not used.

        \var PathPart::propertyName
            The property name to access on the matched object. Used for fixed access after finding
            the object.
    */
struct PathPart
{
    QString id;
    QString typeName;
    QString objectName;

    int index = -1;
    QString propertyName;
};

using Path = QList<PathPart>;

struct RecordingFrame
{
    enum Type { Unknown, Press, Release };
    Q_ENUM(Type);

    QDateTime time;
    Type action;
    Path path;

    Q_GADGET
};

struct Recording
{
    QDateTime start;
    QDateTime end;
    QList<RecordingFrame> frames;

    Q_GADGET
};

/*!
    \class QuickTestEngine
    \ingroup testing
    \brief Provides utilities for testing QML/Quick applications.

    QuickTestEngine offers a set of methods for finding QML objects, simulating user
    interactions, and capturing screenshots in QML-based applications. It uses a path-based
    system to locate and interact with QML items.

    \sa PathPart
*/
class QuickTestEngine : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool eventLogging READ eventLogging WRITE setEventLogging NOTIFY eventLoggingChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged FINAL)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged FINAL)

public:
    /*!
        \brief Constructs a QuickTestEngine with the given \a parent.
    */
    explicit QuickTestEngine(QObject *parent = nullptr);

    bool eventLogging() const;
    void setEventLogging(bool newEventLogging);

    bool enabled() const;
    void setEnabled(bool newEnabled);

    QString address() const;
    void setAddress(const QString &newAddress);

    int port() const;
    void setPort(int newPort);

public slots:

    /*!
        \brief Finds a QML object matching the given path.

        Searches for a QML object starting from the root QML window and descending
        through the QML object hierarchy according to the criteria specified in the
        \a path. Each PathPart in the path represents a step in the search.

        The search begins by finding a QQuickWindow matching the first PathPart.
        Subsequent PathParts are applied to child objects.

        \param path The path specification for finding the object. Must not be empty.
        \return A QVariant containing the found QObject, or an invalid QVariant if
                not found. Check validity with QVariant::isValid().

        \note The path may become fragile if the QML structure changes. Aim for a
              balance between path specificity and path stability.

        \sa findAwait(), PathPart
    */
    QVariant find(const Path &path);

    /*!
        \brief Asynchronously finds a QML object, waiting for its creation.

        Similar to find(), but waits for the object to be created if it is not
        immediately available. This is useful for testing dynamically created
        objects or objects that appear with a delay.

        \param path The path specification for finding the object.
        \param timeout The maximum time in milliseconds to wait for the object
                       to be created.
        \return A QFuture that will contain the found QVariant. The future will
                be resolved when either the object is found or the timeout expires.

        \sa find()
    */
    QFuture<QVariant> findAwait(const Path &path, int timeout);

    /*!
        \brief Simulates a mouse click on the item at the specified path.

        Posts mouse press and release events to simulate a click at the specified
        position relative to the item's size.

        \param path The path to the QQuickItem to click.
        \param relX The horizontal click position relative to the item's width,
                    where 0.0 is the left edge and 1.0 is the right edge. Default is 0.5 (center).
        \param relY The vertical click position relative to the item's height,
                    where 0.0 is the top edge and 1.0 is the bottom edge. Default is 0.5 (center).
        \return \c true if the click was successfully posted; \c false if the path
                did not resolve to a valid QQuickItem or other error occurred.

        \sa mousePress(), mouseRelease()
    */
    bool click(const Path &path, double relX = 0.5, double relY = 0.5);

    /*!
        \brief Simulates a mouse button press on the item at the specified path.

        Posts a mouse press event at the center of the specified item.

        \param path The path to the QQuickItem.
        \return \c true if the press event was successfully posted; \c false if the
                path did not resolve to a valid QQuickItem or other error occurred.

        \sa click(), mouseRelease()
    */
    bool mousePress(const Path &path);

    /*!
        \brief Simulates a mouse button release on the item at the specified path.

        Posts a mouse release event at the center of the specified item.

        \param path The path to the QQuickItem.
        \return \c true if the release event was successfully posted; \c false if the
                path did not resolve to a valid QQuickItem or other error occurred.

        \sa click(), mousePress()
    */
    bool mouseRelease(const Path &path);

    /*!
        \brief Captures a screenshot of the item and encodes it as base64.

        Takes a screenshot using QQuickItem::grabToImage(), saves it in the specified
        format, and returns the result as a base64-encoded byte array.

        \param item The QQuickItem to capture. Must be valid and have an associated window.
        \param format The image format (e.g., "PNG", "JPEG"). Default is "PNG".
        \param quality The compression quality for formats that support it (0-100).
                       Default is 90. Not used for PNG.
        \return A base64-encoded representation of the screenshot image.

        \note The encoding is performed synchronously. For large images, this may
              briefly block the event loop.
    */
    QFuture<QByteArray> takeScreenShotAsBase64(QQuickItem *item, const QString &format = "PNG", int quality = 90);

signals:
    void eventLoggingChanged();
    void enabledChanged();
    void addressChanged();
    void portChanged();

protected:
    /*!
        \brief Checks if an object matches the criteria specified in a PathPart.

        Internal helper function used during object search. Compares the given
        object against all specified criteria in the PathPart.

        \param object The object to check.
        \param pathPart The criteria to match against.
        \return \c true if the object matches all specified criteria; \c false otherwise.

        \internal
    */
    static bool isMatching(QObject *object, const PathPart &pathPart);
    friend class QuickTestEngineTest;

    virtual bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool _eventLogging;
    bool _enabled;
    QString _address;
    int _port;

    Recording _recording;

    ObjectRegistry2 _registry;
    WebSocketServer _webSocketServer;
};

/*!
    \relates QuickTestEngine
    \brief Writes the PathPart \a pathPart to debug output using the \a debug stream.

    Used for debugging and logging path specifications.
*/
QDebug operator<<(QDebug debug, const PathPart &pathPart);

#endif // QUICKTESTENGINE_H
