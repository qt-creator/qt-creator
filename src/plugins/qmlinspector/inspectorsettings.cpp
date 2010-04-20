#include "inspectorsettings.h"
#include "qmlinspectorconstants.h"
#include <QtCore/QSettings>

namespace Qml {
namespace Internal {

InspectorSettings::InspectorSettings() : m_externalPort(3768), m_externalUrl("127.0.0.1"),
m_showUninspectableItems(false), m_showUnwatchableProperties(false), m_groupPropertiesByItemType(true)
{

}

void InspectorSettings::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Qml::Constants::S_QML_INSPECTOR));
    m_externalPort= settings->value(QLatin1String(Qml::Constants::S_EXTERNALPORT_KEY), 3768).toUInt();
    m_externalUrl = settings->value(QLatin1String(Qml::Constants::S_EXTERNALURL_KEY), "127.0.0.1").toString();
    m_showUninspectableItems = settings->value(QLatin1String(Qml::Constants::S_SHOW_UNINSPECTABLE_ITEMS), false).toBool();
    m_showUnwatchableProperties = settings->value(QLatin1String(Qml::Constants::S_SHOW_UNWATCHABLE_PROPERTIES), false).toBool();
    m_groupPropertiesByItemType = settings->value(QLatin1String(Qml::Constants::S_GROUP_PROPERTIES_BY_ITEM_TYPE), true).toBool();
    settings->endGroup();
}

void InspectorSettings::saveSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Qml::Constants::S_QML_INSPECTOR));
    settings->setValue(QLatin1String(Qml::Constants::S_EXTERNALPORT_KEY), m_externalPort);
    settings->setValue(QLatin1String(Qml::Constants::S_EXTERNALURL_KEY), m_externalUrl);
    settings->setValue(QLatin1String(Qml::Constants::S_SHOW_UNINSPECTABLE_ITEMS), m_showUninspectableItems);
    settings->setValue(QLatin1String(Qml::Constants::S_SHOW_UNWATCHABLE_PROPERTIES), m_showUnwatchableProperties);
    settings->setValue(QLatin1String(Qml::Constants::S_GROUP_PROPERTIES_BY_ITEM_TYPE), m_groupPropertiesByItemType);
    settings->endGroup();
}

quint16 InspectorSettings::externalPort() const
{
    return m_externalPort;
}

QString InspectorSettings::externalUrl() const
{
    return m_externalUrl;
}

void InspectorSettings::setExternalPort(quint16 port)
{
    m_externalPort = port;
}

void InspectorSettings::setExternalUrl(const QString &url)
{
    m_externalUrl = url;
}

bool InspectorSettings::showUninspectableItems() const
{
    return m_showUninspectableItems;
}

bool InspectorSettings::showUnwatchableProperties() const
{
    return m_showUnwatchableProperties;
}

bool InspectorSettings::groupPropertiesByItemType() const
{
    return m_groupPropertiesByItemType;
}

void InspectorSettings::setShowUninspectableItems(bool value)
{
    m_showUninspectableItems = value;
}

void InspectorSettings::setShowUnwatchableProperties(bool value)
{
    m_showUnwatchableProperties = value;
}

void InspectorSettings::setGroupPropertiesByItemType(bool value)
{
    m_groupPropertiesByItemType = value;
}

} // Internal
} // Qml
