#include <%QAPP_INCLUDE%>
#include "%INCLUDE%"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    %CLASS% w;
#if defined(Q_WS_S60)
    w.showMaximized();
#else
    w.show();
#endif
    return a.exec();
}
