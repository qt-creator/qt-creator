
#include <QApplication>

extern "C"
void qDumpObjectData440(
    int protocolVersion,
    int token,
    void *data,
    bool dumpChildren,
    int extraInt0,
    int extraInt1,
    int extraInt2,
    int extraInt3);

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    return 0;
}
