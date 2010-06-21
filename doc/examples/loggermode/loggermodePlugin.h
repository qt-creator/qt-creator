#ifndef LOGGERMODE_PLUGIN_H
#define LOGGERMODE_PLUGIN_H

#include <extensionsystem/iplugin.h>

class LoggerMode;
class LoggerModePlugin : public ExtensionSystem::IPlugin
{
public:
    LoggerModePlugin();
    ~LoggerModePlugin();

    void extensionsInitialized();
    bool initialize(const QStringList & arguments, QString * errorString);
    void shutdown();
private:
    LoggerMode *loggerMode;
};

#endif // NEWMODE_PLUGIN_H

