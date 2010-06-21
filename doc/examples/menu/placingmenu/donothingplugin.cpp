#include "donothingplugin.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <QKeySequence>

#include <QtPlugin>
#include <QStringList>
#include <QMessageBox>

DoNothingPlugin::DoNothingPlugin()
{
    // Do nothing
}

DoNothingPlugin::~DoNothingPlugin()
{
    // Do notning
}

void DoNothingPlugin::extensionsInitialized()
{
    // Do nothing
}

bool DoNothingPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    // Fetch the action manager
    Core::ActionManager* am = Core::ICore::instance()->actionManager();

    // Create a DoNothing menu
    Core::ActionContainer* ac = am->createMenu("DoNothingPlugin.DoNothingMenu");
    ac->menu()->setTitle("DoNothing");

    // Create a command for "About DoNothing".
    QAction *action = new QAction(tr("About DoNothing"),this);
    Core::Command* cmd = am->registerAction(action,"DoNothingPlugin.AboutDoNothing",QList<int>() << 0);

    // Insert the "DoNothing" menu between "Window" and "Help".
    QMenu* windowMenu = am->actionContainer(Core::Constants::M_HELP)->menu();
    QMenuBar* menuBar = am->actionContainer(Core::Constants::MENU_BAR)->menuBar();
    menuBar->insertMenu(windowMenu->menuAction(), ac->menu());

    // Add the "About DoNothing" action to the DoNothing menu
    ac->addAction(cmd);

    // Connect the action
    connect(action, SIGNAL(triggered(bool)), this, SLOT(about()));
    return true;
}

void DoNothingPlugin::shutdown()
{
    // Do nothing
}
void DoNothingPlugin::about()
{
    QMessageBox::information(0, "About DoNothing Plugin",
                             "Seriously dude, this plugin does nothing");
}

Q_EXPORT_PLUGIN(DoNothingPlugin)
