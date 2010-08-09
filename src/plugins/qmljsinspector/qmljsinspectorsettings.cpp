#include "qmljsinspectorsettings.h"
#include "qmljsinspectorconstants.h"
#include <QtCore/QSettings>

namespace QmlJSInspector {
namespace Internal {

InspectorSettings::InspectorSettings(QObject *parent)
    : QObject(parent),
      m_showLivePreviewWarning(true)
{
}

InspectorSettings::~InspectorSettings()
{

}

void InspectorSettings::restoreSettings(QSettings *settings)
{

    settings->beginGroup(QLatin1String(QmlJSInspector::Constants::S_QML_INSPECTOR));
    m_showLivePreviewWarning = settings->value(QLatin1String(QmlJSInspector::Constants::S_LIVE_PREVIEW_WARNING_KEY), true).toBool();
    settings->endGroup();
}

void InspectorSettings::saveSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlJSInspector::Constants::S_QML_INSPECTOR));
    settings->setValue(QLatin1String(QmlJSInspector::Constants::S_LIVE_PREVIEW_WARNING_KEY), m_showLivePreviewWarning);
    settings->endGroup();
}


bool InspectorSettings::showLivePreviewWarning() const
{
    return m_showLivePreviewWarning;
}

void InspectorSettings::setShowLivePreviewWarning(bool value)
{
    m_showLivePreviewWarning = value;
}

} // namespace Internal
} // namespace QmlJSInspector
