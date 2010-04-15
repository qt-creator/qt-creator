#ifndef INSPECTORSETTINGS_H
#define INSPECTORSETTINGS_H

#include <QString>
#include "inspectorsettings.h"

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace Qml {
namespace Internal {

class InspectorSettings
{
public:
    InspectorSettings();

    void readSettings(QSettings *settings);
    void saveSettings(QSettings *settings) const;

    void setExternalPort(quint16 port);
    void setExternalUrl(const QString &url);
    quint16 externalPort() const;
    QString externalUrl() const;

private:
    quint16 m_externalPort;
    QString m_externalUrl;

};

} // Internal
} // Qml

#endif // INSPECTORSETTINGS_H
