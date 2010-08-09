#ifndef INSPECTORSETTINGS_H
#define INSPECTORSETTINGS_H

#include <QObject>

QT_FORWARD_DECLARE_CLASS(QSettings)

namespace QmlJSInspector {
namespace Internal {

class InspectorSettings : public QObject
{
public:
    InspectorSettings(QObject *parent = 0);
    ~InspectorSettings();
    void restoreSettings(QSettings *settings);
    void saveSettings(QSettings *settings) const;

    bool showLivePreviewWarning() const;
    void setShowLivePreviewWarning(bool value);

private:
    bool m_showLivePreviewWarning;
};

} // namespace Internal
} // namespace QmlJSInspector

#endif // INSPECTORSETTINGS_H
