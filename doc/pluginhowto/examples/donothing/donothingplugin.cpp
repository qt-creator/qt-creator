#include "donothingplugin.h"

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

    return true;
}

void DoNothingPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(DoNothingPlugin)
