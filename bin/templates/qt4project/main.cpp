#include <%QAPP_INCLUDE%>
#include "%INCLUDE%"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    %CLASS% w;
    w.show();
    return a.exec();
}
