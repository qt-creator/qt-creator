#ifndef DIRMODELPLUGIN_PLUGIN_H
#define DIRMODELPLUGIN_PLUGIN_H

#include <extensionsystem/iplugin.h>

class DirModelPluginPlugin : public ExtensionSystem::IPlugin
{
public:
    DirModelPluginPlugin();
    ~DirModelPluginPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // DIRMODELPLUGIN_PLUGIN_H

