#ifndef DONOTHING_PLUGIN_H
#define DONOTHING_PLUGIN_H

#include <extensionsystem/iplugin.h>

class DoNothingPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DoNothingPlugin();
    ~DoNothingPlugin();
    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // DONOTHING_PLUGIN_H

