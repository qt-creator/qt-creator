%{Cpp:LicenseTemplate}\
#include "%{ConstantsHdrFileName}"
#include "%{TrHdrFileName}"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>

using namespace Core;

namespace %{PluginName}::Internal {

class %{CN} final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "%{PluginName}.json")

public:
    %{CN}() = default;

    ~%{CN}() final
    {
        // Unregister objects from the plugin manager's object pool
        // Other cleanup, if needed.
    }

    void initialize() final
    {
        // Set up this plugin's factories, if needed.
        // Register objects in the plugin manager's object pool, if needed. (rare)
        // Load settings
        // Add actions to menus
        // Connect to other plugins' signals
        // In the initialize function, a plugin can be sure that the plugins it
        // depends on have passed their initialize() phase.

        // If you need access to command line arguments or to report errors, use the
        //    Utils::Result<> IPlugin::initialize(const QStringList &arguments)
        // overload.

        ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
        menu->menu()->setTitle(Tr::tr("%{PluginName}"));
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);

        ActionBuilder(this, Constants::ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("%{PluginName} Action"))
            .setDefaultKeySequence(Tr::tr("Ctrl+Alt+Meta+A"))
            .addOnTriggered(this, &%{CN}::triggerAction);
    }

    void extensionsInitialized() final
    {
        // Retrieve objects from the plugin manager's object pool, if needed. (rare)
        // In the extensionsInitialized function, a plugin can be sure that all
        // plugins that depend on it have passed their initialize() and
        // extensionsInitialized() phase.
    }

    ShutdownFlag aboutToShutdown() final
    {
        // Save settings
        // Disconnect from signals that are not needed during shutdown
        // Hide UI (if you add UI that is not in the main window directly)
        return SynchronousShutdown;
    }

private:
    void triggerAction()
    {
        QMessageBox::information(ICore::dialogParent(),
                                 Tr::tr("Action Triggered"),
                                 Tr::tr("This is an action from %{PluginName}."));
    }
};

} // namespace %{PluginName}::Internal

#include <%{MocFileName}>
