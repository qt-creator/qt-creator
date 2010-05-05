#ifndef %PluginName:u%_%CppHeaderSuffix:u%
#define %PluginName:u%_%CppHeaderSuffix:u%

#include "%PluginName:l%_global.%CppHeaderSuffix%"

#include <extensionsystem/iplugin.h>

namespace %PluginName% {
namespace Internal {

class %PluginName%Impl : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    %PluginName%Impl();
    ~%PluginName%Impl();

    bool initialize(const QStringList &arguments, QString *errorString);
    void extensionsInitialized();
    void aboutToShutdown();
};

} // namespace Internal
} // namespace %PluginName%

#endif // %PluginName:u%_%CppHeaderSuffix:u%
