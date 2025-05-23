#include "setting.h"

#include "unistd.h"

QmlSettings *init()
{
    auto settings = new QmlSettings{geteuid() == 0 ? QSettings::SystemScope : QSettings::UserScope};
    qInfo() << "init settings file:" << settings->fileName();
    return settings;
}

QmlSettings &getApplicationSettings()
{
    static QmlSettings *settings = init();
    return *settings;
}

QmlSettings::QmlSettings(const QString &organization, const QString &application, QObject *parent)
    : QSettings(organization, application, parent)
{}

QmlSettings::QmlSettings(Scope scope, QObject *parent)
    : QSettings(scope, parent)
{}

QmlSettings::QmlSettings(QObject *parent)
    : QSettings(parent)
{}

QmlSettings::QmlSettings(Scope scope, const QString &organization, const QString &application, QObject *parent)
    : QSettings(scope, organization, application, parent)
{}

QmlSettings::QmlSettings(Format format, Scope scope, const QString &organization, const QString &application, QObject *parent)
    : QSettings(format, scope, organization, application, parent)
{}

QmlSettings::QmlSettings(const QString &fileName, Format format, QObject *parent)
    : QSettings(fileName, format, parent)
{}

QmlSetting *QmlSettings::newSetting(const QString &key, const QVariant &defaultValue, QObject *parent)
{
    return new QmlSetting{key, defaultValue, parent};
}

QVariant QmlSettings::value(const QString &key, const QVariant &defaultValue) const
{
    return QSettings::value(key, defaultValue);
}

void QmlSettings::setValue(const QString &key, const QVariant &newValue)
{
    QSettings::setValue(key, newValue);
}

QVariant QmlSetting::value() const
{
    return _value;
}

QmlSetting::QmlSetting(const QString &key, const QVariant &defaultValue, QObject *parent, QSettings *settings)
    : QObject{parent}
    , _settings{settings ? *settings : getApplicationSettings()}
    , _value{_settings.value(key, defaultValue)}
    , _key{key}
{}

void QmlSetting::setValue(const QVariant &newValue)
{
    if (_value == newValue)
        return;
    _value = newValue;
    emit valueChanged();
}

QString QmlSetting::key() const
{
    return _key;
}
