#include "tabpreferences.h"
#include "tabsettings.h"

using namespace TextEditor;

static const char *settingsSuffixKey = "TabPreferences";

static const char *currentFallbackKey = "CurrentFallback";

TabPreferences::TabPreferences(
    const QList<IFallbackPreferences *> &fallbacks, QObject *parent)
    : IFallbackPreferences(fallbacks, parent)
{
    connect(this, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(slotCurrentValueChanged(QVariant)));
}

QVariant TabPreferences::value() const
{
    QVariant v;
    v.setValue(settings());
    return v;
}

void TabPreferences::setValue(const QVariant &value)
{
    if (!value.canConvert<TabSettings>())
        return;

    setSettings(value.value<TabSettings>());
}

TabSettings TabPreferences::settings() const
{
    return m_data;
}

void TabPreferences::setSettings(const TextEditor::TabSettings &data)
{
    if (m_data == data)
        return;

    m_data = data;

    QVariant v;
    v.setValue(data);
    emit valueChanged(v);
    emit settingsChanged(m_data);
    if (!currentFallback()) {
        emit currentValueChanged(v);
    }
}

TabSettings TabPreferences::currentSettings() const
{
    QVariant v = currentValue();
    if (!v.canConvert<TabSettings>()) {
        // warning
        return TabSettings();
    }
    return v.value<TabSettings>();
}

void TabPreferences::slotCurrentValueChanged(const QVariant &value)
{
    if (!value.canConvert<TabSettings>())
        return;

    emit currentSettingsChanged(value.value<TabSettings>());
}

QString TabPreferences::settingsSuffix() const
{
    return settingsSuffixKey;
}

void TabPreferences::toMap(const QString &prefix, QVariantMap *map) const
{
    m_data.toMap(prefix, map);
    map->insert(prefix + QLatin1String(currentFallbackKey), currentFallbackId());
}

void TabPreferences::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_data.fromMap(prefix, map);
    setCurrentFallback(map.value(prefix + QLatin1String(currentFallbackKey), QLatin1String("Global")).toString());
}

