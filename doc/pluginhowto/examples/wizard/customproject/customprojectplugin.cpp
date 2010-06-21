#include "customprojectplugin.h"
#include "customprojectwizard.h"

#include <QtPlugin>
#include <QStringList>

CustomProjectPlugin::CustomProjectPlugin()
{
    // Do nothing
}

CustomProjectPlugin::~CustomProjectPlugin()
{
    // Do notning
}

void CustomProjectPlugin::extensionsInitialized()
{
    // Do nothing
}

bool CustomProjectPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    addAutoReleasedObject(new CustomProjectWizard);

    return true;
}

void CustomProjectPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(CustomProjectPlugin)
