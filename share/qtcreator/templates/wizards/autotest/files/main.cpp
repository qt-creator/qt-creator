%{Cpp:LicenseTemplate}\
@if "%{TestFrameWork}" == "QtTest"
@if "%{RequireGUI}" == "true"
%{JS: QtSupport.qtIncludes([ 'QtGui/QApplication' ],
                           [ 'QtWidgets/QApplication' ]) }\
@else
%{JS: QtSupport.qtIncludes([ 'QtCore/QCoreApplication' ],
                           [ 'QtCore/QCoreApplication' ]) }\
@endif
// add necessary includes here

int main(int argc, char *argv[])
{
@if "%{RequireGUI}" == "true"
    QApplication a(argc, argv);
@else
    QCoreApplication a(argc, argv);
@endif

    return a.exec();
}
@else
#include <iostream>

int main(int , char **)
{
    std::cout << "Hello World!\\n";

    return 0;
}
@endif
