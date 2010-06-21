#include "headerfilterplugin.h"
#include "headerfilter.h"

#include <QtPlugin>
#include <QStringList>

HeaderFilterPlugin::HeaderFilterPlugin()
{
   // Do nothing
}

HeaderFilterPlugin::~HeaderFilterPlugin()
{
    // Do notning
}

void HeaderFilterPlugin::extensionsInitialized()
{
    // Do nothing
}

bool HeaderFilterPlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    addAutoReleasedObject( new HeaderFilter);
    return true;
}

void HeaderFilterPlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(HeaderFilterPlugin)
