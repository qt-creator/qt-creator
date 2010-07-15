#include "donothingplugin.h"
#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/icore.h>
#include <coreplugin/icontext.h>

#include <QKeySequence>
#include <QStringList>
#include <QMessageBox>
#include <QAction>
#include <QMenu>
#include <QtPlugin>

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

    // Create a command for "DoNothing".
    QAction *action = new QAction(tr("DoNothing"),this);
    Core::Command* cmd = am->registerAction(action,
        QLatin1String("DoNothingPlugin.DoNothing"),
        Core::Context(Core::Constants::C_GLOBAL));

    // Add the "DoNothing" action to the tools menu
    am->actionContainer(Core::Constants::M_TOOLS)->addAction(cmd);

    return true;
}

ExtensionSystem::IPlugin::ShutdownFlag DoNothingPlugin::shutdown()
{
    return SynchronousShutdown;
}

Q_EXPORT_PLUGIN(DoNothingPlugin)
