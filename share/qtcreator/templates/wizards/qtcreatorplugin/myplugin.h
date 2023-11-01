#pragma once

#include "%{GlobalHdrFileName}"

#include <extensionsystem/iplugin.h>

namespace %{PluginName}::Internal {

class %{CN} : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "%{PluginName}.json")

public:
    %{CN}();
    ~%{CN}() override;

    void initialize() override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace %{PluginName}::Internal
