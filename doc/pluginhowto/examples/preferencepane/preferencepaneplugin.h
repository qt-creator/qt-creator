#ifndef PREFERENCEPANE_PLUGIN_H
#define PREFERENCEPANE_PLUGIN_H

#include <extensionsystem/iplugin.h>

class PreferencePanePlugin : public ExtensionSystem::IPlugin
{
public:
    PreferencePanePlugin();
    ~PreferencePanePlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // PREFERENCEPANE_PLUGIN_H

