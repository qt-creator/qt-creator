#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalSocket>
#include "lldbengineguest.h"

class DiePlease: public QThread
{
    virtual void run()
    {
        getc(stdin);
        ::exit(0);
    }
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);


    Debugger::Internal::LLDBEngineGuest lldb;

    QLocalSocket s;
    if (argc > 1) {
        // Pure test code. This is not intended for end users.
        // TODO: maybe some day we can have real unit tests for debugger engines?
        setenv("LLDB_DEBUGSERVER_PATH", "/Users/aep/qt-creator/src/plugins/debugger/lldb/lldb/build/Release/LLDB.framework/Resources/debugserver", 0);
        lldb.setupEngine();
        QStringList env;
        env.append(QLatin1String("DYLD_IMAGE_SUFFIX=_debug"));
        lldb.setupInferior(argv[1], QStringList(), env);
        Debugger::Internal::BreakpointParameters bp;
        bp.ignoreCount = 0;
        bp.lineNumber = 64;
        bp.fileName = QLatin1String("main.cpp");
        lldb.addBreakpoint(0, bp);
        lldb.runEngine();
    }else {
        s.connectToServer(QLatin1String("/tmp/qtcreator-debuggeripc"));
        s.waitForConnected();
        app.connect(&s, SIGNAL(disconnected ()), &app, SLOT(quit()));
        lldb.setHostDevice(&s);
    }

    DiePlease bla;
    bla.start();
    return app.exec();
}

extern "C" {
extern const unsigned char lldbVersionString[] __attribute__ ((used)) = "@(#)PROGRAM:lldb  PROJECT:lldb-26" "\n";
extern const double lldbVersionNumber __attribute__ ((used)) = (double)26.;
extern const double LLDBVersionNumber __attribute__ ((used)) = (double)26.;
}
