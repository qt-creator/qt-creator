#pragma once

#include "%PluginName:l%_global.%CppHeaderSuffix%"

#include <extensionsystem/iplugin.h>

namespace %PluginName% {
namespace Internal {

class %PluginName%Plugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "%PluginName%.json")

public:
    %PluginName%Plugin();
    ~%PluginName%Plugin();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

private:
    void triggerAction();
};

} // namespace Internal
} // namespace %PluginName%
