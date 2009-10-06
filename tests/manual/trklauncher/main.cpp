#include "launcher.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QStringList>

static const char *usageC =
"\nUsage: %1 <trk_port_name> [-v] [-i remote_sis_file | -I local_sis_file remote_sis_file] [<remote_executable_name>]\n"
"\nOptions:\n    -v verbose\n"
            "    -f turn serial message frame off\n\n"
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

static bool parseArguments(const QStringList &arguments, trk::Launcher &launcher)
{
    // Parse away options
    bool install = false;
    bool customInstall = false;
    const int argCount = arguments.size();
    int verbosity = 0;
    int a = 1;
    for ( ; a < argCount; a++) {
        const QString option = arguments.at(a);
        if (!option.startsWith(QLatin1Char('-')))
            break;
        if (option.size() != 2)
            return  false;
        switch (option.at(1).toAscii()) {
        case 'v':
            verbosity++;
            break;
        case 'f':
            launcher.setSerialFrame(false);
            break;verbosity++;
        case 'i':
            install = true;
            launcher.addStartupActions(trk::Launcher::ActionInstall);
            break;
        case 'I':
            customInstall = true;
            launcher.addStartupActions(trk::Launcher::ActionCopyInstall);
            break;
        default:
            return false;
        }
    }

    launcher.setVerbose(verbosity);
    // Evaluate arguments
    const int remainingArgsCount = argCount - a;
    if (remainingArgsCount == 1 && !install && !customInstall) {
        launcher.setTrkServerName(arguments.at(a)); // ping
        return true;
    }
    if (remainingArgsCount == 2 && !install && !customInstall) {
        // remote exec
        launcher.addStartupActions(trk::Launcher::ActionRun);
        launcher.setTrkServerName(arguments.at(a));
        launcher.setFileName(arguments.at(a + 1));
        return true;
    }
    if ((remainingArgsCount == 3 || remainingArgsCount == 2) && install && !customInstall) {
        launcher.setTrkServerName(arguments.at(a)); // ping
        launcher.setInstallFileName(arguments.at(a + 1));
        if (remainingArgsCount == 3) {
            launcher.addStartupActions(trk::Launcher::ActionRun);
            launcher.setFileName(arguments.at(a + 2));
        }
        return true;
    }
    if ((remainingArgsCount == 4 || remainingArgsCount == 3) && !install && customInstall) {
        launcher.setTrkServerName(arguments.at(a)); // ping
        launcher.setCopyFileName(arguments.at(a + 1), arguments.at(a + 2));
        launcher.setInstallFileName(arguments.at(a + 2));
        if (remainingArgsCount == 4) {
            launcher.addStartupActions(trk::Launcher::ActionRun);
            launcher.setFileName(arguments.at(a + 3));
        }
        return true;
    }
    return false;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QLatin1String("trklauncher"));
    QCoreApplication::setOrganizationName(QLatin1String("Nokia"));

    trk::Launcher launcher;
    if (!parseArguments(app.arguments(), launcher)) {
        usage();
        return 1;
    }
    QObject::connect(&launcher, SIGNAL(finished()), &app, SLOT(quit()));
    QString errorMessage;
    if (launcher.startServer(&errorMessage))
        return app.exec();
    qWarning("%s\n", qPrintable(errorMessage));
    return 4;
}

