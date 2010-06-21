#ifndef HEADERFILTER_PLUGIN_H
#define HEADERFILTER_PLUGIN_H

#include <extensionsystem/iplugin.h>

class HeaderFilterPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    HeaderFilterPlugin();
    ~HeaderFilterPlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
};

#endif // HEADERFILTER_PLUGIN_H

