#include <%QAPP_INCLUDE%>
#include "%INCLUDE%"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    %CLASS% w;
%SHOWMETHOD%
    return a.exec();
}
