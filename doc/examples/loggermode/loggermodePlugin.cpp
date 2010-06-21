#include "loggermodePlugin.h"

#include "loggermode.h"

#include <QtPlugin>
#include <QStringList>

LoggerModePlugin::LoggerModePlugin()
{
}

LoggerModePlugin::~LoggerModePlugin()
{
}

void LoggerModePlugin::extensionsInitialized()
{
}

bool LoggerModePlugin::initialize(const QStringList& args, QString *errMsg)
{
    Q_UNUSED(args);
    Q_UNUSED(errMsg);

    loggerMode = new LoggerMode;
    addAutoReleasedObject(loggerMode);

    return true;
}

void LoggerModePlugin::shutdown()
{
    // Do nothing
}

Q_EXPORT_PLUGIN(LoggerModePlugin)
