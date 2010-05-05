#include "%PluginName:l%.%CppHeaderSuffix%"

#include <QtPlugin>

using namespace %PluginName%::Internal;

%PluginName%Impl::%PluginName%Impl()
{
    // Create your members
}

%PluginName%Impl::~%PluginName%Impl()
{
    // Unregister objects from the plugin manager's object pool
    // Delete members
}

bool %PluginName%Impl::initialize(const QStringList &arguments, QString *errorString)
{
    // Register objects in the plugin manager's object pool
    // Load settings
    // Add actions to menus
    // connect to other plugins' signals
    // "In the initialize method, a plugin can be sure that the plugins it
    //  depends on have initialized their members."
    return true;
}

void %PluginName%Impl::extensionsInitialized()
{
    // Retrieve objects from the plugin manager's object pool
    // "In the extensionsInitialized method, a plugin can be sure that all
    //  plugins that depend on it are completely initialized."
}

void %PluginName%Impl::aboutToShutdown()
{
    // Save settings
    // Disconnect from signals that are not needed during shutdown
}

Q_EXPORT_PLUGIN2(%PluginName%, %PluginName%Impl)
