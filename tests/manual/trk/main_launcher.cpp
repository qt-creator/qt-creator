#include "launcher.h"

using namespace trk;

int main(int argc, char *argv[])
{
    if ((argc != 3 && argc != 5 && argc != 6)
            || (argc == 5 && QString(argv[2]) != "-i")
            || (argc == 6 && QString(argv[2]) != "-I")) {
        qDebug() << "Usage: " << argv[0] << "<trk_port_name> [-i remote_sis_file | -I local_sis_file remote_sis_file] <remote_executable_name>";
        qDebug() << "for example" << argv[0] << "COM5 C:\\sys\\bin\\test.exe";
        qDebug() << "           " << argv[0] << "COM5 -i C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe";
        qDebug() << "           " << argv[0] << "COM5 -I C:\\Projects\\test\\test_gcce_udeb.sisx C:\\Data\\test_gcce_udeb.sisx C:\\sys\\bin\\test.exe";
        return 1;
    }

    QCoreApplication app(argc, argv);

    Adapter adapter;
    adapter.setTrkServerName(argv[1]);
    if (argc == 3) {
        adapter.setFileName(argv[2]);
    } else if (argc == 5) {
        adapter.setInstallFileName(argv[3]);
        adapter.setFileName(argv[4]);
    } else {
        adapter.setCopyFileName(argv[3], argv[4]);
        adapter.setInstallFileName(argv[4]);
        adapter.setFileName(argv[5]);
    }
    QObject::connect(&adapter, SIGNAL(finished()), &app, SLOT(quit()));
    if (adapter.startServer())
        return app.exec();
    return 4;
}

