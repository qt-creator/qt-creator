#include "inspectorsettings.h"
#include "qmlinspectorconstants.h"
#include <QtCore/QSettings>

namespace Qml {
namespace Internal {

InspectorSettings::InspectorSettings() : m_externalPort(3768), m_externalUrl("127.0.0.1")
{

}

void InspectorSettings::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(Qml::Constants::S_QML_INSPECTOR));
    m_externalPort= settings->value(QLatin1String(Qml::Constants::S_EXTERNALPORT_KEY), 3768).toUInt();
    m_externalUrl = settings->value(QLatin1String(Qml::Constants::S_EXTERNALURL_KEY), "127.0.0.1").toString();
    settings->endGroup();
}

void InspectorSettings::saveSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(Qml::Constants::S_QML_INSPECTOR));
    settings->setValue(QLatin1String(Qml::Constants::S_EXTERNALPORT_KEY), m_externalPort);
    settings->setValue(QLatin1String(Qml::Constants::S_EXTERNALURL_KEY), m_externalUrl);
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

} // Internal
} // Qml
