
#include <QApplication>
#include <QProcess>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QStringList args = app.arguments();

    int n = args.size() == 1 ? 3 : args.at(0).toInt() - 1;
    if (n >= 0) {
        QProcess proc;
        proc.startDetached(args.at(0), QStringList(QString::number(n)));
    }
}
