%{Cpp:LicenseTemplate}\

#include "setup.h"

void Setup::applicationAvailable()
{
    // custom code that does not require QQmlEngine
}

void Setup::qmlEngineAvailable(QQmlEngine *engine)
{
    // custom code that needs QQmlEngine, register QML types, add import paths,...
}

void Setup::cleanupTestCase()
{
    // custom code to clean up before destruction starts
}
