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

    // Create a command for "About DoNothing".
    QAction *action = new QAction(tr("About DoNothing"),this);
    Core::Command* cmd = am->registerAction(action,"DoNothingPlugin.AboutDoNothing",QList<int>() << 0);
    Core::ActionContainer* ac = am->createMenu("DoNothingPlugin.DoNothingMenu");

    // Add the command to Help menu
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd);

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
