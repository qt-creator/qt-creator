#ifndef PROGRESSBAR_PLUGIN_H
#define PROGRESSBAR_PLUGIN_H

#include <extensionsystem/iplugin.h>

class ProgressBarPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    ProgressBarPlugin();
    ~ProgressBarPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString* errorString);
    void shutdown();
};

#endif // PROGRESSBAR_PLUGIN_H

