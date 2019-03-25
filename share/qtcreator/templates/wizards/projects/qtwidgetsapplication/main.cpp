%{Cpp:LicenseTemplate}\
#include "%{HdrFileName}"

%{JS: QtSupport.qtIncludes([ 'QtGui/QApplication' ], [ 'QtWidgets/QApplication' ]) }\

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    %{Class} w;
    w.show();
    return a.exec();
}
