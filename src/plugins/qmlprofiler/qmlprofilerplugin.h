#ifndef QMLPROFILERPLUGIN_H
#define QMLPROFILERPLUGIN_H

#include "qmlprofiler_global.h"

#include <extensionsystem/iplugin.h>

namespace Analyzer {
namespace Internal {

class QmlProfilerPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    static QmlProfilerPlugin *instance();

    QmlProfilerPlugin();
    ~QmlProfilerPlugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    static bool debugOutput;

private:
    class QmlProfilerPluginPrivate;
    QmlProfilerPluginPrivate *d;

    static QmlProfilerPlugin *m_instance;
};

} // namespace Internal
} // namespace Analyzer

#endif // QMLPROFILERPLUGIN_H

