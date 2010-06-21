#include "donothingplugin.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <QKeySequence>

#include <QtPlugin>
#include <QStringList>

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
    Core::Command* cmd = am->registerAction(new QAction(tr("About DoNothing"),this),"DoNothingPlugin.AboutDoNothing",
                                            QList<int>() <<Core::Constants::C_GLOBAL_ID);

    // Add the command to Help menu
    am->actionContainer(Core::Constants::M_HELP)->addAction(cmd);
    return true;
}

void DoNothingPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(DoNothingPlugin)
