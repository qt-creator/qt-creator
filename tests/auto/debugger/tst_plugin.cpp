#include <ctype.h>

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QMetaMethod>
#include <QtCore/QModelIndex>
#if QT_VERSION >= 0x040500
#include <QtCore/QSharedPointer>
#endif
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QtGui/QApplication>
#include <QtGui/QDialog>
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QStandardItemModel>
#include <QtGui/QStringListModel>
#include <QtTest/QtTest>

//#include <QtTest/qtest_gui.h>

#include <QtCore/private/qobject_p.h>

#include "gdb/gdbmi.h"
#include "gdbmacros.h"
#include "gdbmacros_p.h"

#undef NS
#ifdef QT_NAMESPACE
#   define STRINGIFY0(s) #s
#   define STRINGIFY1(s) STRINGIFY0(s)
#   define NS STRINGIFY1(QT_NAMESPACE) "::"
#else
#   define NS ""
#endif

using namespace Debugger;
using namespace Debugger::Internal;


static QByteArray stripped(QByteArray ba)
{
    for (int i = ba.size(); --i >= 0; ) {
        if (ba.at(i) == '\n' || ba.at(i) == ' ')
            ba.chop(1);
        else
            break;
    }
    return ba;
}


class tst_Plugin : public QObject
{
    Q_OBJECT

public:
    tst_Plugin() {}


public slots:
    void runQtc();

public slots:
    void readStandardOutput();
    void readStandardError();

private:
    QProcess m_proc; // the Qt Creator process
};

void tst_Plugin::readStandardOutput()
{
    qDebug() << "qtcreator-out: " << stripped(m_proc.readAllStandardOutput());
}

void tst_Plugin::readStandardError()
{
    qDebug() << "qtcreator-err: " << stripped(m_proc.readAllStandardError());
}

void tst_Plugin::runQtc()
{
    QString test = QFileInfo(qApp->arguments().at(0)).absoluteFilePath();
    QString qtc = QFileInfo(test).absolutePath() + "/../../../bin/qtcreator.bin";
    qtc = QFileInfo(qtc).absoluteFilePath();
    QStringList env = QProcess::systemEnvironment();
    env.append("QTC_DEBUGGER_TEST=" + test);
    m_proc.setEnvironment(env);
    connect(&m_proc, SIGNAL(readyReadStandardOutput()),
        this, SLOT(readStandardOutput()));
    connect(&m_proc, SIGNAL(readyReadStandardError()),
        this, SLOT(readStandardError()));
    m_proc.start(qtc);
    m_proc.waitForStarted();
    QCOMPARE(m_proc.state(), QProcess::Running);
    m_proc.waitForFinished();
    QCOMPARE(m_proc.state(), QProcess::NotRunning);
}

void runDebuggee()
{
    qDebug() << "RUNNING DEBUGGEE";
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();

    if (args.size() == 2 && args.at(1) == "--run-debuggee") {
        runDebuggee();
        app.exec();
        return 0;
    }

    tst_Plugin test;
    return QTest::qExec(&test, argc, argv);
}

#include "tst_plugin.moc"

