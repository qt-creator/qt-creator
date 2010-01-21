#include "launcher.h"
#include "communicationstarter.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QSharedPointer>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

static const char *usageC =
"\n"
"Usage: %1 [options] <trk_port_name>\n"
"       %1 [options] -i <trk_port_name> remote_sis_file\n"
"       %1 [options] -I local_sis_file remote_sis_file] [<remote_executable_name>]\n"
"\nOptions:\n    -v verbose\n"
            "    -b Prompt for Bluetooth connect (Linux only)\n"
            "    -f turn serial message frame off (Bluetooth)\n"
"\nPing:\n"
"%1 COM5\n"
"\nRemote launch:\n"
"%1 COM5 C:\\sys\\bin\\test.exe\n"
"\nInstallation:\n"
"%1 -i COM5 C:\\Data\\test_gcce_udeb.sisx\n"
"\nInstallation and remote launch:\n"
"%1 -i COM5 C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe\n"
"\nCopy from local file, installation:\n"
"%1 -I COM5 C:\\Projects\\test\\test_gcce_udeb.sisx C:\\Data\\test_gcce_udeb.sisx\n"
"\nCopy from local file, installation and remote launch:\n"
"%1 -I COM5 C:\\Projects\\test\\test_gcce_udeb.sisx C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe\n";

static void usage()
{
    const QString msg = QString::fromLatin1(usageC).arg(QCoreApplication::applicationName());
    qWarning("%s", qPrintable(msg));
}

typedef QSharedPointer<trk::Launcher> TrkLauncherPtr;

// Parse arguments, return pointer or a null none.

static inline TrkLauncherPtr createLauncher(trk::Launcher::Actions actions,
                                            const QString &serverName,
                                            bool serialFrame,
                                            int verbosity)
{
    TrkLauncherPtr launcher(new trk::Launcher(actions));
    launcher->setTrkServerName(serverName);
    launcher->setSerialFrame(serialFrame);
    launcher->setVerbose(verbosity);
    return launcher;
}

static TrkLauncherPtr parseArguments(const QStringList &arguments, bool *bluetooth)
{
    // Parse away options
    bool install = false;
    bool customInstall = false;
    bool serialFrame = true;
    const int argCount = arguments.size();
    int verbosity = 0;
    *bluetooth = false;
    trk::Launcher::Actions actions = trk::Launcher::ActionPingOnly;
    int a = 1;
    for ( ; a < argCount; a++) {
        const QString option = arguments.at(a);
        if (!option.startsWith(QLatin1Char('-')))
            break;
        if (option.size() != 2)
            return TrkLauncherPtr();        
        switch (option.at(1).toAscii()) {
        case 'v':
            verbosity++;
            break;
        case 'f':
            serialFrame = false;
            break;
        case 'b':
            *bluetooth = true;
            break;
        case 'i':
            install = true;
            actions = trk::Launcher::ActionInstall;
            break;
        case 'I':
            customInstall = true;
            actions = trk::Launcher::ActionCopyInstall;
            break;
        default:
            return TrkLauncherPtr();
        }
    }
    // Evaluate arguments
    const int remainingArgsCount = argCount - a;
    if (remainingArgsCount == 1 && !install && !customInstall) { // Ping
        return createLauncher(actions, arguments.at(a), serialFrame, verbosity);
    }
    if (remainingArgsCount == 2 && !install && !customInstall) {
        // remote exec
        TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
        launcher->addStartupActions(trk::Launcher::ActionRun);
        launcher->setFileName(arguments.at(a + 1));
        return launcher;
    }
    if ((remainingArgsCount == 3 || remainingArgsCount == 2) && install && !customInstall) {
        TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
        launcher->setInstallFileName(arguments.at(a + 1));
        if (remainingArgsCount == 3) {
            launcher->addStartupActions(trk::Launcher::ActionRun);
            launcher->setFileName(arguments.at(a + 2));
        }
        return launcher;
    }
    if ((remainingArgsCount == 4 || remainingArgsCount == 3) && !install && customInstall) {
        TrkLauncherPtr launcher = createLauncher(actions, arguments.at(a), serialFrame, verbosity);
        launcher->setTrkServerName(arguments.at(a)); // ping
        launcher->setCopyFileName(arguments.at(a + 1), arguments.at(a + 2));
        launcher->setInstallFileName(arguments.at(a + 2));
        if (remainingArgsCount == 4) {
            launcher->addStartupActions(trk::Launcher::ActionRun);
            launcher->setFileName(arguments.at(a + 3));
        }
        return launcher;
    }
    return TrkLauncherPtr();
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("TRKlauncher"));
    QCoreApplication::setOrganizationName(QLatin1String("Nokia"));

    bool bluetooth;
    const TrkLauncherPtr launcher = parseArguments(app.arguments(), &bluetooth);
    if (launcher.isNull()) {
        usage();
        return 1;
    }
    QObject::connect(launcher.data(), SIGNAL(finished()), &app, SLOT(quit()));
    // BLuetooth: Open with prompt
    QString errorMessage;
    if (bluetooth && !trk::ConsoleBluetoothStarter::startBluetooth(launcher->trkDevice(),
                                                     launcher.data(),
                                                     launcher->trkServerName(),
                                                     30, &errorMessage)) {
        qWarning("%s\n", qPrintable(errorMessage));
        return -1;
    }
    if (launcher->startServer(&errorMessage))
        return app.exec();
    qWarning("%s\n", qPrintable(errorMessage));
    return 4;
}

