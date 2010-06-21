#include "progressbarplugin.h"
#include "headerfilterprogress.h"

#include <QtPlugin>
#include <QStringList>

ProgressBarPlugin::ProgressBarPlugin()
{
   // Do nothing
}

ProgressBarPlugin::~ProgressBarPlugin()
{
    // Do notning
}

void ProgressBarPlugin::extensionsInitialized()
{
    // Do nothing
}

bool ProgressBarPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    addAutoReleasedObject( new HeaderFilterProgress);
    return true;
}

void ProgressBarPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(ProgressBarPlugin)
