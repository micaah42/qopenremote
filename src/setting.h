#ifndef SETTING_H
#define SETTING_H

#include <QSettings>

QSettings &getApplicationSettings();

template<class T>
class Setting
{
public:
    explicit Setting(const QString &key, const T &defaultValue, QSettings *settings = nullptr);

    operator const T &() const;
    const T &operator*() const;
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
