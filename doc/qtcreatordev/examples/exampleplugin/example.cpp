#include "exampleconstants.h"
#include "examplefunctions.h"
#include "exampletr.h"

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

//! [test include]
#ifdef WITH_TESTS
#include <QtTest>
#endif
//! [test include]

using namespace Core;

//! [namespaces]
namespace Example::Internal {
//! [namespaces]

//! [plugin tests]
#ifdef WITH_TESTS
class MyPluginTests : public QObject
{
    Q_OBJECT

private slots:
    void testMyTest()
    {
        // a failing test
        QVERIFY(false);
    }
};
#endif
//! [plugin tests]

//! [base class]
class ExamplePlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Example.json")
//! [base class]

public:
    ExamplePlugin() = default;

    ~ExamplePlugin() final
    {
        // Unregister objects from the plugin manager's object pool
        // Other cleanup, if needed.
    }

//! [initialize]
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
//! [initialize]

//! [add menu]
        ActionContainer *menu = ActionManager::createMenu(Constants::MENU_ID);
        menu->menu()->setTitle(Tr::tr("Example"));
        ActionManager::actionContainer(Core::Constants::M_TOOLS)->addMenu(menu);
//! [add menu]

//! [add action]
        ActionBuilder(this, Constants::ACTION_ID)
            .addToContainer(Constants::MENU_ID)
            .setText(Tr::tr("Example Action"))
            .setDefaultKeySequence(Tr::tr("Ctrl+Alt+Meta+A"))
            .addOnTriggered(this, &ExamplePlugin::triggerAction);
//! [add action]

//! [register tests]
#ifdef WITH_TESTS
        addTest<MyPluginTests>();
#endif
//! [register tests]
    }

//! [plugin functions]
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
//! [plugin functions]

//! [slot implementation]
private:
    void triggerAction()
    {
        QMessageBox::information(ICore::dialogParent(),
                                 Tr::tr("Action Triggered"),
                                 Tr::tr("This is an action from Example."));
    }
//! [slot implementation]
};

} // namespace Example::Internal

namespace Example {

//! [exported function]
int addOne(int i)
{
    return i; // that is wrong!
}
//! [exported function]

} // namespace Example

//! [include moc]
#include "example.moc"
//! [include moc]
