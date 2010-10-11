#include "argumentscollector.h"
#include "sftptest.h"

#include <coreplugin/ssh/sftpchannel.h>
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
    const Parameters parameters
        = ArgumentsCollector(app.arguments()).collect(parseSuccess);
    if (!parseSuccess)
        return EXIT_FAILURE;
    SftpTest sftpTest(parameters);
    sftpTest.run();
    return app.exec();
}
