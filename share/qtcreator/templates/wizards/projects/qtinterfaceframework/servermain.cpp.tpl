#include "servermain.h"

#include "%{FeatureLowerCase}adapter.h"
#include "%{FeatureLowerCase}backend.h"

#include <QCoreApplication>

void serverMain(QIfRemoteObjectsConfig &config)
{
    %{Feature}Backend *backend = new %{Feature}Backend(QCoreApplication::instance());
    %{Feature}Adapter *adapter = new %{Feature}Adapter(backend);
    config.enableRemoting(u"%{ProjectNameCap}.%{ProjectNameCap}Module"_s, u"%{Feature}"_s, adapter);
}
