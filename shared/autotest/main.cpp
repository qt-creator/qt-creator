@if "%RequireGUI%" == "true"
#include <QApplication>
@else
#include <QCoreApplication>
@endif

// add necessary includes here

int main(int argc, char *argv[])
{
@if "%RequireGUI%" == "true"
    QApplication a(argc, argv);
@else
    QCoreApplication a(argc, argv);
@endif

    return a.exec();
}
