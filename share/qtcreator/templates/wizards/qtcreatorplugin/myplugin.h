@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GUARD}
#define %{GUARD}
@endif

#include "%{GlobalHdrFileName}"

#include <extensionsystem/iplugin.h>

namespace %{PluginName} {
namespace Internal {

class %{CN} : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "%{PluginName}.json")

public:
    %{CN}();
    ~%{CN}() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace Internal
} // namespace %{PluginName}

@if ! '%{Cpp:PragmaOnce}'
#endif // %{GUARD}
@endif
