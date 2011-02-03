#include "cppcodestylepreferences.h"

using namespace CppTools;

static const char *settingsSuffixKey = "CodeStyleSettings";

static const char *currentFallbackKey = "CurrentFallback";

CppCodeStylePreferences::CppCodeStylePreferences(const QList<TextEditor::IFallbackPreferences *> &fallbacks, QObject *parent) :
    IFallbackPreferences(fallbacks, parent)
{
    connect(this, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(slotCurrentValueChanged(QVariant)));
}

QVariant CppCodeStylePreferences::value() const
{
    QVariant v;
    v.setValue(settings());
    return v;
}

void CppCodeStylePreferences::setValue(const QVariant &data)
{
    if (!data.canConvert<CppCodeStyleSettings>())
        return;

    setSettings(data.value<CppCodeStyleSettings>());
}

CppCodeStyleSettings CppCodeStylePreferences::settings() const
{
    return m_data;
}

void CppCodeStylePreferences::setSettings(const CppCodeStyleSettings &data)
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

CppCodeStyleSettings CppCodeStylePreferences::currentSettings() const
{
    QVariant v = currentValue();
    if (!v.canConvert<CppCodeStyleSettings>()) {
        // warning
        return CppCodeStyleSettings();
    }
    return v.value<CppCodeStyleSettings>();
}

void CppCodeStylePreferences::slotCurrentValueChanged(const QVariant &value)
{
    if (!value.canConvert<CppCodeStyleSettings>())
        return;

    emit currentSettingsChanged(value.value<CppCodeStyleSettings>());
}

QString CppCodeStylePreferences::settingsSuffix() const
{
    return settingsSuffixKey;
}

void CppCodeStylePreferences::toMap(const QString &prefix, QVariantMap *map) const
{
    m_data.toMap(prefix, map);
    map->insert(prefix + QLatin1String(currentFallbackKey), currentFallbackId());
}

void CppCodeStylePreferences::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_data.fromMap(prefix, map);
    setCurrentFallback(map.value(prefix + QLatin1String(currentFallbackKey), QLatin1String("Global")).toString());
}

