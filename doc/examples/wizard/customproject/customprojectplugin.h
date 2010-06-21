#ifndef CUSTOMPROJECT_PLUGIN_H
#define CUSTOMPROJECT_PLUGIN_H

#include <extensionsystem/iplugin.h>

class CustomProjectPlugin : public ExtensionSystem::IPlugin
{
public:
    CustomProjectPlugin();
    ~CustomProjectPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // CUSTOMPROJECT_PLUGIN_H

