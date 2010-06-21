#include "preferencepaneplugin.h"
#include "modifiedfilelister.h"

#include <QtPlugin>
#include <QStringList>

PreferencePanePlugin::PreferencePanePlugin()
{
    // Do nothing
}

PreferencePanePlugin::~PreferencePanePlugin()
{
    // Do notning
}

void PreferencePanePlugin::extensionsInitialized()
{
    // Do nothing
}

bool PreferencePanePlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    addAutoReleasedObject(new ModifiedFileLister);
    return true;
}

void PreferencePanePlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(PreferencePanePlugin)
