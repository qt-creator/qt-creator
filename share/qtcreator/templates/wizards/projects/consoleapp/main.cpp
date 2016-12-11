%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([ 'QtCore/QCoreApplication' ],
                           [ 'QtCore/QCoreApplication' ]) }\

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    return a.exec();
}
