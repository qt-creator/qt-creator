@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %ProjectName:h%_H
#define %ProjectName:h%_H
@endif

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
    ~%PluginName%Plugin() override;

    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

private:
    void triggerAction();
};

} // namespace Internal
} // namespace %PluginName%

@if ! '%{Cpp:PragmaOnce}'
#endif // %ProjectName:h%_H
@endif
