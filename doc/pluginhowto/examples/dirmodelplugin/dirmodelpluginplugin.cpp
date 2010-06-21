#include "dirmodelpluginplugin.h"

#include <QtPlugin>
#include <QStringList>
#include "DirNavigationFactory.h"

DirModelPluginPlugin::DirModelPluginPlugin()
{
    // Do nothing
}

DirModelPluginPlugin::~DirModelPluginPlugin()
{
    // Do notning
}

void DirModelPluginPlugin::extensionsInitialized()
{
    // Do nothing
}

bool DirModelPluginPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);
    addAutoReleasedObject(new DirNavigationFactory);
    return true;
}

void DirModelPluginPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(DirModelPluginPlugin)
