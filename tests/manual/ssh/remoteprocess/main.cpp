#include "argumentscollector.h"
#include "remoteprocesstest.h"

#include <coreplugin/ssh/sshconnection.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QStringList>

#include <cstdlib>
#include <iostream>

using namespace Core;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    bool parseSuccess;
    const Core::SshConnectionParameters &parameters
        = ArgumentsCollector(app.arguments()).collect(parseSuccess);
    if (!parseSuccess)
        return EXIT_FAILURE;
    RemoteProcessTest remoteProcessTest(parameters);
    remoteProcessTest.run();
    return app.exec();
}
