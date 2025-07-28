#ifndef SETTING_H
#define SETTING_H

#include <QSettings>
#include <QVariant>

class QmlSetting : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariant value READ value WRITE setValue NOTIFY valueChanged FINAL)
    Q_PROPERTY(QString key READ key CONSTANT FINAL)

public:
    explicit QmlSetting(const QString &key, const QVariant &defaultValue = QVariant(), QObject *parent = nullptr, QSettings *settings = nullptr);

    void setValue(const QVariant &newValue);
    QVariant value() const;
    QString key() const;

signals:
    void valueChanged();

private:
    QSettings &_settings;
    QVariant _value;
    QString _key;
};

class QmlSettings : public QSettings
{
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName CONSTANT FINAL)
public:
    explicit QmlSettings(const QString &organization, const QString &application = QString(), QObject *parent = nullptr);
    explicit QmlSettings(Scope scope, QObject *parent = nullptr);
    explicit QmlSettings(QObject *parent = nullptr);

    QmlSettings(Scope scope, const QString &organization, const QString &application = QString(), QObject *parent = nullptr);
    QmlSettings(Format format, Scope scope, const QString &organization, const QString &application = QString(), QObject *parent = nullptr);
    QmlSettings(const QString &fileName, Format format, QObject *parent = nullptr);

public slots:
    QmlSetting *newSetting(const QString &key, const QVariant &defaultValue, QObject *parent);
    QVariant value(const QString &key, const QVariant &defaultValue) const;
    void setValue(const QString &key, const QVariant &newValue);
};

QmlSettings &getApplicationSettings();

template<class T>
class Setting
{
public:
    explicit Setting(const QString &key, const T &defaultValue, QSettings *settings = nullptr);

    operator const T &() const;
    const T &operator*() const;
    bool operator==(const T &t);
    ;
    void operator=(const T &t);

private:
    QSettings &_settings;
    const QString _key;
    T _defaultValue;
    T _value;
};

template<class T>
inline Setting<T>::Setting(const QString &key, const T &defaultValue, QSettings *settings)
    : _settings{settings ? *settings : getApplicationSettings()}
    , _key{key}
    , _defaultValue{defaultValue}
    , _value{_settings.value(key, QVariant::fromValue(defaultValue)).template value<T>()}
{}

template<class T>
inline const T &Setting<T>::operator*() const
{
    return _value;
}

template<class T>
inline bool Setting<T>::operator==(const T &t)
{
    return _value == t;
}

template<class T>
inline void Setting<T>::operator=(const T &t)
{
    _settings.setValue(_key, QVariant::fromValue(t));
    _value = t;
}

template<class T>
inline Setting<T>::operator const T &() const
{
    return _value;
}
#endif // SETTING_H
