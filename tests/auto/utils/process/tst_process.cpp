// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processtestapp/processtestapp.h"

#include <app/app_version.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/processinfo.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>
#include <utils/singleton.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QtTest>

using namespace Utils;

using namespace std::chrono;
using namespace std::chrono_literals;

// This handler is inspired by the one used in qtbase/tests/auto/corelib/io/qfile/tst_qfile.cpp
class MessageHandler {
public:
    MessageHandler(QtMessageHandler messageHandler = handler)
    {
        s_oldMessageHandler = qInstallMessageHandler(messageHandler);
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(s_oldMessageHandler);
    }

    static int destroyCount()
    {
        return s_destroyCount;
    }

protected:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (msg.contains("QProcess: Destroyed while process")
                && msg.contains("is still running.")) {
            ++s_destroyCount;
        }
        // Defer to old message handler.
        if (s_oldMessageHandler)
            s_oldMessageHandler(type, context, msg);
    }

    static QtMessageHandler s_oldMessageHandler;
    static int s_destroyCount;
};

int MessageHandler::s_destroyCount = 0;
QtMessageHandler MessageHandler::s_oldMessageHandler = 0;

static constexpr char s_skipTerminateOnWindows[] =
        "Windows implementation of this test is lacking handling of WM_CLOSE message.";

class tst_Process : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // Keep me as a first test to ensure that all singletons are working OK
    // when being instantiated from the non-main thread.
    void processReaperCreatedInNonMainThread();
    void testEnv()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("Skipping env test on Windows");

        QProcess qproc;
        FilePath envPath = Environment::systemEnvironment().searchInPath("env");

        qproc.setProgram(envPath.nativePath());
        qproc.start();
        qproc.waitForFinished();
        QByteArray qoutput = qproc.readAllStandardOutput() + qproc.readAllStandardError();
        qDebug() << "QProcess output:" << qoutput;
        QCOMPARE(qproc.exitCode(), 0);

        Process proc;
        proc.setCommand(CommandLine{envPath});
        proc.runBlocking();
        QCOMPARE(proc.exitCode(), 0);
        const QByteArray output = proc.rawStdOut() + proc.rawStdErr();
        qDebug() << "Process output:" << output;

        QCOMPARE(output.size() > 0, qoutput.size() > 0);
    }

    void multiRead_data();
    void multiRead();
    void splitArgs_data();
    void splitArgs();
    void prepareArgs_data();
    void prepareArgs();
    void prepareArgsEnv_data();
    void prepareArgsEnv();
    void iterations_data();
    void iterations();
    void iteratorEditsWindows();
    void iteratorEditsLinux();
    void exitCode_data();
    void exitCode();
    void runBlockingStdOut_data();
    void runBlockingStdOut();
    void runBlockingSignal_data();
    void runBlockingSignal();
    void lineCallback();
    void lineSignal();
    void waitForStartedAfterStarted();
    void waitForStartedAfterStarted2();
    void waitForStartedAndFinished();
    void notRunningAfterStartingNonExistingProgram_data();
    void notRunningAfterStartingNonExistingProgram();
    void channelForwarding_data();
    void channelForwarding();
    void mergedChannels_data();
    void mergedChannels();
    void destroyBlockingProcess_data();
    void destroyBlockingProcess();
    void flushFinishedWhileWaitingForReadyRead_data();
    void flushFinishedWhileWaitingForReadyRead();
    void crash();
    void crashAfterOneSecond();
    void recursiveCrashingProcess();
    void recursiveBlockingProcess();
    void quitBlockingProcess_data();
    void quitBlockingProcess();
    void tarPipe();
    void stdinToShell();
    void eventLoopMode_data();
    void eventLoopMode();

    void cleanupTestCase();

private:
    void iteratorEditsHelper(OsType osType);

    Environment envWindows;
    Environment envLinux;

    QString homeStr;
    QString home;

    MessageHandler *msgHandler = nullptr;
};

void tst_Process::initTestCase()
{
    msgHandler = new MessageHandler;
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    SubProcessConfig::setPathToProcessTestApp(QLatin1String(PROCESS_TESTAPP));

    homeStr = QLatin1String("@HOME@");
    home = QDir::homePath();

    QStringList env;
    env << "empty=" << "word=hi" << "words=hi ho" << "spacedwords= hi   ho sucker ";
    envWindows = Environment(env, OsTypeWindows);
    envLinux = Environment(env, OsTypeLinux);
}

void tst_Process::cleanupTestCase()
{
    Singleton::deleteAll();
    const int destroyCount = msgHandler->destroyCount();
    delete msgHandler;
    if (destroyCount)
        qDebug() << "Received" << destroyCount << "messages about destroying running QProcess!";
    QCOMPARE(destroyCount, 0);
}

Q_DECLARE_METATYPE(ProcessArgs::SplitError)
Q_DECLARE_METATYPE(OsType)
Q_DECLARE_METATYPE(ProcessResult)

static bool deleteRunningProcess()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);
    process.start();
    process.waitForStarted();
    return process.isRunning();
}

void tst_Process::processReaperCreatedInNonMainThread()
{
    Singleton::deleteAll();

    auto future = Utils::asyncRun(deleteRunningProcess);
    future.waitForFinished();
    QVERIFY(future.result());

    Singleton::deleteAll();
}

void tst_Process::multiRead_data()
{
    QTest::addColumn<QProcess::ProcessChannel>("processChannel");

    QTest::newRow("StandardOutput") << QProcess::StandardOutput;
    QTest::newRow("StandardError") << QProcess::StandardError;
}

static QByteArray readData(Process *process, QProcess::ProcessChannel processChannel)
{
    return processChannel == QProcess::StandardOutput ? process->readAllRawStandardOutput()
                                                      : process->readAllRawStandardError();
}

void tst_Process::multiRead()
{
    QFETCH(QProcess::ProcessChannel, processChannel);

    SubProcessConfig subConfig(ProcessTestApp::ChannelEchoer::envVar(),
                               QString::number(int(processChannel)));

    QByteArray buffer;
    Process process;
    subConfig.setupSubProcess(&process);

    process.setProcessMode(ProcessMode::Writer);
    process.start();

    QVERIFY(process.waitForStarted());

    process.writeRaw("hi\n");
    QVERIFY(process.waitForReadyRead(1s));
    buffer = readData(&process, processChannel);
    QCOMPARE(buffer, QByteArray("hi"));

    process.writeRaw("you\n");
    QVERIFY(process.waitForReadyRead(1s));
    buffer = readData(&process, processChannel);
    QCOMPARE(buffer, QByteArray("you"));

    process.writeRaw("exit\n");
    QVERIFY(process.waitForFinished(1s));
}

void tst_Process::splitArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<ProcessArgs::SplitError>("err");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const ProcessArgs::SplitError err;
        const OsType os;
    } vals[] = {
        {"", "", ProcessArgs::SplitOk, OsTypeWindows},
        {" ", "", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {" hi ho ", "hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi ho\" \"hi\" ho  ", "\"hi ho\" hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\", "\\", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\"", "\"\"\\^\"\"\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi\"\"\"ho\"", "\"hi\"\\^\"\"ho\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\\\\\"", "\"\"\\\\\\^\"\"\"", ProcessArgs::SplitOk, OsTypeWindows},
        {" ^^ ", "\"^^\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi\"", "", ProcessArgs::BadQuoting, OsTypeWindows},
        {"hi\"dood", "", ProcessArgs::BadQuoting, OsTypeWindows},
        {"%var%", "%var%", ProcessArgs::SplitOk, OsTypeWindows},

        {"", "", ProcessArgs::SplitOk, OsTypeLinux},
        {" ", "", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" hi ho ", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {"'hi ho' \"hi\" ho  ", "'hi ho' hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" \\ ", "' '", ProcessArgs::SplitOk, OsTypeLinux},
        {" \\\" ", "'\"'", ProcessArgs::SplitOk, OsTypeLinux},
        {" '\"' ", "'\"'", ProcessArgs::SplitOk, OsTypeLinux},
        {" \"\\\"\" ", "'\"'", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi'", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"hi\"dood", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"$var", "'$var'", ProcessArgs::SplitOk, OsTypeLinux},
        {"~", "@HOME@", ProcessArgs::SplitOk, OsTypeLinux},
        {"~ foo", "@HOME@ foo", ProcessArgs::SplitOk, OsTypeLinux},
        {"foo ~", "foo @HOME@", ProcessArgs::SplitOk, OsTypeLinux},
        {"~/foo", "@HOME@/foo", ProcessArgs::SplitOk, OsTypeLinux},
        {"~foo", "'~foo'", ProcessArgs::SplitOk, OsTypeLinux}
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_Process::splitArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(ProcessArgs::SplitError, err);
    QFETCH(OsType, os);

    ProcessArgs::SplitError outerr;
    QString outstr = ProcessArgs::joinArgs(ProcessArgs::splitArgs(in, os, false, &outerr), os);
    QCOMPARE(outerr, err);
    if (err == ProcessArgs::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_Process::prepareArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<ProcessArgs::SplitError>("err");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const ProcessArgs::SplitError err;
        const OsType os;
    } vals[] = {
        {" ", " ", ProcessArgs::SplitOk, OsTypeWindows},
        {"", "", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {" hi ho ", " hi ho ", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\", "\\", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\"", "\\\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi\"\"ho\"", "\"hi\"\"ho\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\\\\\"", "\\\\\\\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"^^", "^", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi\"", "hi\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi\"dood", "hi\"dood", ProcessArgs::SplitOk, OsTypeWindows},
        {"%var%", "", ProcessArgs::FoundMeta, OsTypeWindows},
        {"echo hi > file", "", ProcessArgs::FoundMeta, OsTypeWindows},

        {"", "", ProcessArgs::SplitOk, OsTypeLinux},
        {" ", "", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" hi ho ", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {"'hi ho' \"hi\" ho  ", "'hi ho' hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" \\ ", "' '", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi'", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"hi\"dood", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"$var", "", ProcessArgs::FoundMeta, OsTypeLinux},
        {"~", "@HOME@", ProcessArgs::SplitOk, OsTypeLinux},
        {"~ foo", "@HOME@ foo", ProcessArgs::SplitOk, OsTypeLinux},
        {"~/foo", "@HOME@/foo", ProcessArgs::SplitOk, OsTypeLinux},
        {"~foo", "", ProcessArgs::FoundMeta, OsTypeLinux}
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_Process::prepareArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(ProcessArgs::SplitError, err);
    QFETCH(OsType, os);

    ProcessArgs::SplitError outerr;
    ProcessArgs args = ProcessArgs::prepareArgs(in, &outerr, os);
    QString outstr = args.toString();

    QCOMPARE(outerr, err);
    if (err == ProcessArgs::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_Process::prepareArgsEnv_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<ProcessArgs::SplitError>("err");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        const ProcessArgs::SplitError err;
        const OsType os;
    } vals[] = {
        {" ", " ", ProcessArgs::SplitOk, OsTypeWindows},
        {"", "", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {" hi ho ", " hi ho ", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi ho\" \"hi\" ho  ", "\"hi ho\" \"hi\" ho  ", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\", "\\", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\"", "\\\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\"hi\"\"ho\"", "\"hi\"\"ho\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"\\\\\\\"", "\\\\\\\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"^^", "^", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi\"", "hi\"", ProcessArgs::SplitOk, OsTypeWindows},
        {"hi\"dood", "hi\"dood", ProcessArgs::SplitOk, OsTypeWindows},
        {"%empty%", "%empty%", ProcessArgs::SplitOk, OsTypeWindows}, // Yep, no empty variables on Windows.
        {"%word%", "hi", ProcessArgs::SplitOk, OsTypeWindows},
        {" %word% ", " hi ", ProcessArgs::SplitOk, OsTypeWindows},
        {"%words%", "hi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"%nonsense%words%", "%nonsensehi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"fail%nonsense%words%", "fail%nonsensehi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"%words%words%", "hi howords%", ProcessArgs::SplitOk, OsTypeWindows},
        {"%words%%words%", "hi hohi ho", ProcessArgs::SplitOk, OsTypeWindows},
        {"echo hi > file", "", ProcessArgs::FoundMeta, OsTypeWindows},

        {"", "", ProcessArgs::SplitOk, OsTypeLinux},
        {" ", "", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi ho", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" hi ho ", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {"'hi ho' \"hi\" ho  ", "'hi ho' hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {" \\ ", "' '", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi'", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"hi\"dood", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"$empty", "", ProcessArgs::SplitOk, OsTypeLinux},
        {"$word", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {" $word ", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {"${word}", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {" ${word} ", "hi", ProcessArgs::SplitOk, OsTypeLinux},
        {"$words", "hi ho", ProcessArgs::SplitOk, OsTypeLinux},
        {"$spacedwords", "hi ho sucker", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi${empty}ho", "hiho", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi${words}ho", "hihi hoho", ProcessArgs::SplitOk, OsTypeLinux},
        {"hi${spacedwords}ho", "hi hi ho sucker ho", ProcessArgs::SplitOk, OsTypeLinux},
        {"${", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"${var", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"${var ", "", ProcessArgs::FoundMeta, OsTypeLinux},
        {"\"hi${words}ho\"", "'hihi hoho'", ProcessArgs::SplitOk, OsTypeLinux},
        {"\"hi${spacedwords}ho\"", "'hi hi   ho sucker ho'", ProcessArgs::SplitOk, OsTypeLinux},
        {"\"${", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"\"${var", "", ProcessArgs::BadQuoting, OsTypeLinux},
        {"\"${var ", "", ProcessArgs::FoundMeta, OsTypeLinux},
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        QString out = QString::fromLatin1(vals[i].out);
        if (vals[i].os == OsTypeLinux)
            out.replace(homeStr, home);
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << out << vals[i].err << vals[i].os;
    }
}

void tst_Process::prepareArgsEnv()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(ProcessArgs::SplitError, err);
    QFETCH(OsType, os);

    ProcessArgs::SplitError outerr;
    ProcessArgs args = ProcessArgs::prepareArgs(in, &outerr, os, os == OsTypeLinux ? &envLinux : &envWindows);
    QString outstr = args.toString();

    QCOMPARE(outerr, err);
    if (err == ProcessArgs::SplitOk)
        QCOMPARE(outstr, out);
}



void tst_Process::iterations_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<OsType>("os");

    static const struct {
        const char * const in;
        const char * const out;
        OsType os;
    } vals[] = {
        {"", "", OsTypeWindows},
        {"hi", "hi", OsTypeWindows},
        {"  hi ", "hi", OsTypeWindows},
        {"hi ho", "hi ho", OsTypeWindows},
        {"\"hi ho\" sucker", "\"hi ho\" sucker", OsTypeWindows},
        {"\"hi\"^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker", OsTypeWindows},
        {"\"hi\"\\^\"\"ho\" sucker", "\"hi\"\\^\"\"ho\" sucker", OsTypeWindows},
        {"hi^|ho", "\"hi|ho\"", OsTypeWindows},
        {"c:\\", "c:\\", OsTypeWindows},
        {"\"c:\\\\\"", "c:\\", OsTypeWindows},
        {"\\hi\\ho", "\\hi\\ho", OsTypeWindows},
        {"hi null%", "hi null%", OsTypeWindows},
        {"hi null% ho", "hi null% ho", OsTypeWindows},
        {"hi null%here ho", "hi null%here ho", OsTypeWindows},
        {"hi null%here%too ho", "hi {} ho", OsTypeWindows},
        {"echo hello | more", "echo hello", OsTypeWindows},
        {"echo hello| more", "echo hello", OsTypeWindows},

        {"", "", OsTypeLinux},
        {" ", "", OsTypeLinux},
        {"hi", "hi", OsTypeLinux},
        {"  hi ", "hi", OsTypeLinux},
        {"'hi'", "hi", OsTypeLinux},
        {"hi ho", "hi ho", OsTypeLinux},
        {"\"hi ho\" sucker", "'hi ho' sucker", OsTypeLinux},
        {"\"hi\\\"ho\" sucker", "'hi\"ho' sucker", OsTypeLinux},
        {"\"hi'ho\" sucker", "'hi'\\''ho' sucker", OsTypeLinux},
        {"'hi ho' sucker", "'hi ho' sucker", OsTypeLinux},
        {"\\\\", "'\\'", OsTypeLinux},
        {"'\\'", "'\\'", OsTypeLinux},
        {"hi 'null${here}too' ho", "hi 'null${here}too' ho", OsTypeLinux},
        {"hi null${here}too ho", "hi {} ho", OsTypeLinux},
        {"hi $(echo $dollar cent) ho", "hi {} ho", OsTypeLinux},
        {"hi `echo $dollar \\`echo cent\\` | cat` ho", "hi {} ho", OsTypeLinux},
        {"echo hello | more", "echo hello", OsTypeLinux},
        {"echo hello| more", "echo hello", OsTypeLinux},
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out)
                                  << vals[i].os;
}

void tst_Process::iterations()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(OsType, os);

    QString outstr;
    for (ProcessArgs::ArgIterator ait(&in, os); ait.next(); ) {
        if (ait.isSimple())
            ProcessArgs::addArg(&outstr, ait.value(), os);
        else
            ProcessArgs::addArgs(&outstr, "{}");
    }
    QCOMPARE(outstr, out);
}

void tst_Process::iteratorEditsHelper(OsType osType)
{
    QString in1 = "one two three", in2 = in1, in3 = in1, in4 = in1, in5 = in1;

    ProcessArgs::ArgIterator ait1(&in1, osType);
    QVERIFY(ait1.next());
    ait1.deleteArg();
    QVERIFY(ait1.next());
    QVERIFY(ait1.next());
    QVERIFY(!ait1.next());
    QCOMPARE(in1, QString::fromLatin1("two three"));
    ait1.appendArg("four");
    QCOMPARE(in1, QString::fromLatin1("two three four"));

    ProcessArgs::ArgIterator ait2(&in2, osType);
    QVERIFY(ait2.next());
    QVERIFY(ait2.next());
    ait2.deleteArg();
    QVERIFY(ait2.next());
    ait2.appendArg("four");
    QVERIFY(!ait2.next());
    QCOMPARE(in2, QString::fromLatin1("one three four"));

    ProcessArgs::ArgIterator ait3(&in3, osType);
    QVERIFY(ait3.next());
    ait3.appendArg("one-b");
    QVERIFY(ait3.next());
    QVERIFY(ait3.next());
    ait3.deleteArg();
    QVERIFY(!ait3.next());
    QCOMPARE(in3, QString::fromLatin1("one one-b two"));

    ProcessArgs::ArgIterator ait4(&in4, osType);
    ait4.appendArg("pre-one");
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    QVERIFY(ait4.next());
    ait4.deleteArg();
    QVERIFY(!ait4.next());
    QCOMPARE(in4, QString::fromLatin1("pre-one one two"));

    ProcessArgs::ArgIterator ait5(&in5, osType);
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(ait5.next());
    QVERIFY(!ait5.next());
    ait5.deleteArg();
    QVERIFY(!ait5.next());
    QCOMPARE(in5, QString::fromLatin1("one two"));
}

void tst_Process::iteratorEditsWindows()
{
    iteratorEditsHelper(OsTypeWindows);
}

void tst_Process::iteratorEditsLinux()
{
    iteratorEditsHelper(OsTypeLinux);
}

void tst_Process::exitCode_data()
{
    QTest::addColumn<int>("exitCode");

    static const auto exitCodes = {
#ifdef Q_OS_WIN
        "99999999", "-255", "-1",
#endif // Q_OS_WIN
        "0", "1", "255"
    };
    for (auto exitCode : exitCodes)
        QTest::newRow(exitCode) << QString::fromLatin1(exitCode).toInt();
}

void tst_Process::exitCode()
{
    QFETCH(int, exitCode);

    SubProcessConfig subConfig(ProcessTestApp::ExitCode::envVar(), QString::number(exitCode));
    {
        Process process;
        subConfig.setupSubProcess(&process);
        process.start();
        const bool finished = process.waitForFinished();

        QVERIFY(finished);
        QCOMPARE(process.exitCode(), exitCode);
        QCOMPARE(process.exitCode() == 0, process.result() == ProcessResult::FinishedWithSuccess);
    }
    {
        Process process;
        subConfig.setupSubProcess(&process);
        process.runBlocking();

        QCOMPARE(process.exitCode(), exitCode);
        QCOMPARE(process.exitCode() == 0, process.result() == ProcessResult::FinishedWithSuccess);
    }
}

void tst_Process::runBlockingStdOut_data()
{
    QTest::addColumn<bool>("withEndl");
    QTest::addColumn<seconds>("timeout");
    QTest::addColumn<ProcessResult>("expectedResult");

    // Canceled, since the process is killed (canceled) from the callback.
    QTest::newRow("Short timeout with end of line") << true << 1s << ProcessResult::Canceled;

    // Canceled, since it times out.
    QTest::newRow("Short timeout without end of line") << false << 1s << ProcessResult::Canceled;

    // FinishedWithSuccess, since it doesn't time out, it finishes process normally,
    // calls the callback handler and tries to stop the process forcefully what is no-op
    // at this point in time since the process is already finished.
    QTest::newRow("Long timeout without end of line")
            << false << 10s << ProcessResult::FinishedWithSuccess;
}

void tst_Process::runBlockingStdOut()
{
    QFETCH(bool, withEndl);
    QFETCH(seconds, timeout);
    QFETCH(ProcessResult, expectedResult);

    SubProcessConfig subConfig(ProcessTestApp::RunBlockingStdOut::envVar(), withEndl ? "true" : "false");
    Process process;
    subConfig.setupSubProcess(&process);

    bool readLastLine = false;
    process.setStdOutCallback([&readLastLine, &process](const QString &out) {
        if (out.contains(s_runBlockingStdOutSubProcessMagicWord)) {
            readLastLine = true;
            process.kill();
        }
    });
    process.runBlocking(timeout);

    // See also QTCREATORBUG-25667 for why it is a bad idea to use Process::runBlocking
    // with interactive cli tools.
    QCOMPARE(process.result(), expectedResult);
    QVERIFY2(readLastLine, "Last line was read.");
}

void tst_Process::runBlockingSignal_data()
{
    runBlockingStdOut_data();
}

void tst_Process::runBlockingSignal()
{
    QFETCH(bool, withEndl);
    QFETCH(seconds, timeout);
    QFETCH(ProcessResult, expectedResult);

    SubProcessConfig subConfig(ProcessTestApp::RunBlockingStdOut::envVar(), withEndl ? "true" : "false");
    Process process;
    subConfig.setupSubProcess(&process);

    bool readLastLine = false;
    process.setTextChannelMode(Channel::Output, TextChannelMode::MultiLine);
    connect(&process, &Process::textOnStandardOutput,
            this, [&readLastLine, &process](const QString &out) {
        if (out.contains(s_runBlockingStdOutSubProcessMagicWord)) {
            readLastLine = true;
            process.kill();
        }
    });
    process.runBlocking(timeout);

    // See also QTCREATORBUG-25667 for why it is a bad idea to use Process::runBlocking
    // with interactive cli tools.
    QCOMPARE(process.result(), expectedResult);
    QVERIFY2(readLastLine, "Last line was read.");
}

void tst_Process::lineCallback()
{
    SubProcessConfig subConfig(ProcessTestApp::LineCallback::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    const QStringList lines = QString(s_lineCallbackData).split('|');
    int lineNumber = 0;
    process.setStdErrLineCallback([lines, &lineNumber](const QString &actual) {
        QString expected = lines.at(lineNumber);
        expected.replace("\r\n", "\n");
        // Omit some initial lines generated by Qt, e.g.
        // Warning: Ignoring WAYLAND_DISPLAY on Gnome. Use QT_QPA_PLATFORM=wayland to run on Wayland anyway.
        if (lineNumber == 0 && actual != expected)
            return;
        ++lineNumber;
        QCOMPARE(actual, expected);
    });
    process.start();
    process.waitForFinished();
    QCOMPARE(lineNumber, lines.size());
}

void tst_Process::lineSignal()
{
    SubProcessConfig subConfig(ProcessTestApp::LineCallback::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    const QStringList lines = QString(s_lineCallbackData).split('|');
    int lineNumber = 0;
    process.setTextChannelMode(Channel::Error, TextChannelMode::SingleLine);
    connect(&process, &Process::textOnStandardError,
            this, [lines, &lineNumber](const QString &actual) {
        QString expected = lines.at(lineNumber);
        expected.replace("\r\n", "\n");
        // Omit some initial lines generated by Qt, e.g.
        // Warning: Ignoring WAYLAND_DISPLAY on Gnome. Use QT_QPA_PLATFORM=wayland to run on Wayland anyway.
        if (lineNumber == 0 && actual != expected)
            return;
        ++lineNumber;
        QCOMPARE(actual, expected);
    });
    process.start();
    process.waitForFinished();
    QCOMPARE(lineNumber, lines.size());
}

void tst_Process::waitForStartedAfterStarted()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    bool started = false;
    bool waitForStartedResult = false;
    connect(&process, &Process::started, this, [&] {
        started = true;
        waitForStartedResult = process.waitForStarted();
    });

    process.start();
    QVERIFY(process.waitForFinished());
    QVERIFY(started);
    QVERIFY(waitForStartedResult);
    QVERIFY(!process.waitForStarted());
}

// This version is using QProcess
void tst_Process::waitForStartedAfterStarted2()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    QProcess process;
    subConfig.setupSubProcess(&process);

    bool started = false;
    bool waitForStartedResult = false;
    connect(&process, &QProcess::started, this, [&] {
        started = true;
        waitForStartedResult = process.waitForStarted();
    });

    process.start();
    QVERIFY(process.waitForFinished());
    QVERIFY(started);
    QVERIFY(waitForStartedResult);
    QVERIFY(!process.waitForStarted());
}

void tst_Process::waitForStartedAndFinished()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    process.start();
    QThread::msleep(1000); // long enough for process to finish
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished());
    QVERIFY(!process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
}

Q_DECLARE_METATYPE(ProcessSignalType)

void tst_Process::notRunningAfterStartingNonExistingProgram_data()
{
    QTest::addColumn<ProcessSignalType>("signalType");

    QTest::newRow("Started") << ProcessSignalType::Started;
    QTest::newRow("ReadyRead") << ProcessSignalType::ReadyRead;
    QTest::newRow("Done") << ProcessSignalType::Done;
}

void tst_Process::notRunningAfterStartingNonExistingProgram()
{
    QFETCH(ProcessSignalType, signalType);

    Process process;
    process.setCommand(CommandLine{"there_is_a_big_chance_that_executable_with_that_name_does_not_exists"});

    int doneCount = 0;
    QObject::connect(&process, &Process::done, [&process, &doneCount]() {
        ++doneCount;
        QCOMPARE(process.error(), QProcess::FailedToStart);
    });

    const int loopCount = 2;
    for (int i = 0; i < loopCount; ++i) {
        // Work on the same process instance on every iteration
        process.start();

        QElapsedTimer timer;
        timer.start();
        const seconds timeout = 1s;

        switch (signalType) {
        case ProcessSignalType::Started: QVERIFY(!process.waitForStarted(timeout)); break;
        case ProcessSignalType::ReadyRead: QVERIFY(!process.waitForReadyRead(timeout)); break;
        case ProcessSignalType::Done: QVERIFY(!process.waitForFinished(timeout)); break;
        }

        // shouldn't wait, should finish immediately
        QVERIFY(timer.elapsed() < duration_cast<milliseconds>(timeout).count());
        QCOMPARE(process.state(), QProcess::NotRunning);
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.error(), QProcess::FailedToStart);
        QVERIFY(process.exitCode() != 0);
        QCOMPARE(process.result(), ProcessResult::StartFailed);
    }
    QCOMPARE(doneCount, loopCount);
}

// Since we want to test whether the process forwards its channels or not, we can't just create
// a process and start it, because in this case there is no way on how to check whether something
// went into our output channels or not.

// So we start two processes in chain instead. On the beginning the channelForwarding()
// test starts the ChannelForwarding::main() - this one will start another process
// StandardOutputAndErrorWriter::main() with forwarding options.
// The StandardOutputAndErrorWriter::main() is very simple - it just puts something to the output
// and the error channels. Then ChannelForwarding::main() either forwards these channels
// or not - we check it in the outer channelForwarding() test.

void tst_Process::channelForwarding_data()
{
    QTest::addColumn<QProcess::ProcessChannelMode>("channelMode");
    QTest::addColumn<bool>("outputForwarded");
    QTest::addColumn<bool>("errorForwarded");

    QTest::newRow("SeparateChannels") << QProcess::SeparateChannels << false << false;
    QTest::newRow("MergedChannels") << QProcess::MergedChannels << false << false;
    QTest::newRow("ForwardedChannels") << QProcess::ForwardedChannels << true << true;
    QTest::newRow("ForwardedOutputChannel") << QProcess::ForwardedOutputChannel << true << false;
    QTest::newRow("ForwardedErrorChannel") << QProcess::ForwardedErrorChannel << false << true;
}

void tst_Process::channelForwarding()
{
    QFETCH(QProcess::ProcessChannelMode, channelMode);
    QFETCH(bool, outputForwarded);
    QFETCH(bool, errorForwarded);

    SubProcessConfig subConfig(ProcessTestApp::ChannelForwarding::envVar(),
                               QString::number(int(channelMode)));
    Process process;
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForFinished());

    const QByteArray output = process.rawStdOut();
    const QByteArray error = process.rawStdErr();

    QCOMPARE(output.contains(QByteArray(s_outputData)), outputForwarded);
    QCOMPARE(error.contains(QByteArray(s_errorData)), errorForwarded);
}

void tst_Process::mergedChannels_data()
{
    QTest::addColumn<QProcess::ProcessChannelMode>("channelMode");
    QTest::addColumn<bool>("outputOnOutput");
    QTest::addColumn<bool>("outputOnError");
    QTest::addColumn<bool>("errorOnOutput");
    QTest::addColumn<bool>("errorOnError");

    QTest::newRow("SeparateChannels") << QProcess::SeparateChannels
                  << true << false << false << true;
    QTest::newRow("MergedChannels") << QProcess::MergedChannels
                  << true << false << true << false;
    QTest::newRow("ForwardedChannels") << QProcess::ForwardedChannels
                  << false << false << false << false;
    QTest::newRow("ForwardedOutputChannel") << QProcess::ForwardedOutputChannel
                  << false << false << false << true;
    QTest::newRow("ForwardedErrorChannel") << QProcess::ForwardedErrorChannel
                  << true << false << false << false;
}

void tst_Process::mergedChannels()
{
    QFETCH(QProcess::ProcessChannelMode, channelMode);
    QFETCH(bool, outputOnOutput);
    QFETCH(bool, outputOnError);
    QFETCH(bool, errorOnOutput);
    QFETCH(bool, errorOnError);

    SubProcessConfig subConfig(ProcessTestApp::StandardOutputAndErrorWriter::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    process.setProcessChannelMode(channelMode);
    process.start();
    QVERIFY(process.waitForFinished());

    const QByteArray output = process.readAllRawStandardOutput();
    const QByteArray error = process.readAllRawStandardError();

    QCOMPARE(output.contains(QByteArray(s_outputData)), outputOnOutput);
    QCOMPARE(error.contains(QByteArray(s_outputData)), outputOnError);
    QCOMPARE(output.contains(QByteArray(s_errorData)), errorOnOutput);
    QCOMPARE(error.contains(QByteArray(s_errorData)), errorOnError);
}

void tst_Process::destroyBlockingProcess_data()
{
    QTest::addColumn<BlockType>("blockType");

    QTest::newRow("EndlessLoop") << BlockType::EndlessLoop;
    QTest::newRow("InfiniteSleep") << BlockType::InfiniteSleep;
    QTest::newRow("MutexDeadlock") << BlockType::MutexDeadlock;
    QTest::newRow("EventLoop") << BlockType::EventLoop;
}

void tst_Process::destroyBlockingProcess()
{
    QFETCH(BlockType, blockType);

    SubProcessConfig subConfig(ProcessTestApp::BlockingProcess::envVar(),
                               QString::number(int(blockType)));

    Process process;
    subConfig.setupSubProcess(&process);
    process.start();
    QVERIFY(process.waitForStarted());
    QVERIFY(process.isRunning());
    QVERIFY(!process.waitForFinished(1s));
}

void tst_Process::flushFinishedWhileWaitingForReadyRead_data()
{
    QTest::addColumn<QProcess::ProcessChannel>("processChannel");
    QTest::addColumn<QByteArray>("expectedData");

    QTest::newRow("StandardOutput") << QProcess::StandardOutput << QByteArray(s_outputData);
    QTest::newRow("StandardError") << QProcess::StandardError << QByteArray(s_errorData);
}

void tst_Process::flushFinishedWhileWaitingForReadyRead()
{
    QFETCH(QProcess::ProcessChannel, processChannel);
    QFETCH(QByteArray, expectedData);

    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(),
                               QString::number(int(processChannel)));
    Process process;
    subConfig.setupSubProcess(&process);

    process.start();

    QVERIFY(process.waitForStarted());
    QCOMPARE(process.state(), QProcess::Running);

    QByteArray reply;
    while (process.state() == QProcess::Running) {
        process.waitForReadyRead();
        if (processChannel == QProcess::StandardOutput)
            reply += process.readAllRawStandardOutput();
        else
            reply += process.readAllRawStandardError();
    }
    QVERIFY(reply.contains(expectedData));
}

void tst_Process::crash()
{
    SubProcessConfig subConfig(ProcessTestApp::Crash::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForStarted(1s));
    QVERIFY(process.isRunning());

    QEventLoop loop;
    connect(&process, &Process::done, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(process.error(), QProcess::Crashed);
    QCOMPARE(process.exitStatus(), QProcess::CrashExit);
}

void tst_Process::crashAfterOneSecond()
{
    SubProcessConfig subConfig(ProcessTestApp::CrashAfterOneSecond::envVar(), {});
    Process process;
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForStarted(1s));
    QElapsedTimer timer;
    timer.start();
    QVERIFY(process.waitForFinished(30s));
    QVERIFY(timer.elapsed() < 30000); // in milliseconds
    QCOMPARE(process.state(), QProcess::NotRunning);
    QCOMPARE(process.error(), QProcess::Crashed);
}

void tst_Process::recursiveCrashingProcess()
{
    const int recursionDepth = 5; // must be at least 2
    SubProcessConfig subConfig(ProcessTestApp::RecursiveCrashingProcess::envVar(),
                               QString::number(recursionDepth));
    Process process;
    subConfig.setupSubProcess(&process);
    process.start();
    QVERIFY(process.waitForStarted(1s));
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.state(), QProcess::NotRunning);
    QCOMPARE(process.exitStatus(), QProcess::NormalExit);
    QCOMPARE(process.exitCode(), s_crashCode);
}

static int runningTestProcessCount()
{
    int testProcessCounter = 0;
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList();
    for (const ProcessInfo &processInfo : processInfoList) {
        if (FilePath::fromString(processInfo.executable).baseName() == "processtestapp")
            ++testProcessCounter;
    }
    return testProcessCounter;
}

void tst_Process::recursiveBlockingProcess()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP(s_skipTerminateOnWindows);

    Singleton::deleteAll();
    QCOMPARE(runningTestProcessCount(), 0);
    const int recursionDepth = 5; // must be at least 2
    SubProcessConfig subConfig(ProcessTestApp::RecursiveBlockingProcess::envVar(),
                               QString::number(recursionDepth));
    {
        Process process;
        QSignalSpy readSpy(&process, &Process::readyReadStandardOutput);
        QSignalSpy doneSpy(&process, &Process::done);
        subConfig.setupSubProcess(&process);
        process.start();
        QTRY_COMPARE(readSpy.count(), 1); // Wait until 1st ready read signal comes.
        QCOMPARE(process.readAllRawStandardOutput(), s_leafProcessStarted);
        QCOMPARE(runningTestProcessCount(), recursionDepth);
        QCOMPARE(doneSpy.count(), 0);
        process.terminate();
        QTRY_COMPARE(readSpy.count(), 2); // Wait until 2nd ready read signal comes.
        QCOMPARE(process.readAllRawStandardOutput(), s_leafProcessTerminated);
        QTRY_COMPARE(doneSpy.count(), 1); // Wait until done signal comes.
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), s_crashCode);
    }
    Singleton::deleteAll();
    QCOMPARE(runningTestProcessCount(), 0);
}

enum class QuitType {
    Terminate,
    Kill,
    Stop,
    Close
};

Q_DECLARE_METATYPE(QuitType)

void tst_Process::quitBlockingProcess_data()
{
    QTest::addColumn<QuitType>("quitType");
    QTest::addColumn<bool>("doneExpected");
    QTest::addColumn<bool>("gracefulQuit");

    QTest::newRow("Terminate") << QuitType::Terminate << true << true;
    QTest::newRow("Kill") << QuitType::Kill << true << false;
    QTest::newRow("Stop") << QuitType::Stop << true << true;
    QTest::newRow("Close") << QuitType::Close << false << true;
}

void tst_Process::quitBlockingProcess()
{
    QFETCH(QuitType, quitType);
    QFETCH(bool, doneExpected);
    QFETCH(bool, gracefulQuit);

    if (HostOsInfo::isWindowsHost() && quitType == QuitType::Terminate)
        QSKIP(s_skipTerminateOnWindows);

    const int recursionDepth = 1;

    SubProcessConfig subConfig(ProcessTestApp::RecursiveBlockingProcess::envVar(),
                               QString::number(recursionDepth));

    Process process;
    QSignalSpy readSpy(&process, &Process::readyReadStandardOutput);
    QSignalSpy doneSpy(&process, &Process::done);
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForStarted());
    QCOMPARE(doneSpy.count(), 0);
    QVERIFY(process.isRunning());

    QTRY_COMPARE(readSpy.count(), 1); // Wait until ready read signal comes.
    QCOMPARE(process.readAllRawStandardOutput(), s_leafProcessStarted);

    switch (quitType) {
    case QuitType::Terminate: process.terminate(); break;
    case QuitType::Kill: process.kill(); break;
    case QuitType::Stop: process.stop(); break;
    case QuitType::Close: process.close(); break;
    }

    QCOMPARE(doneSpy.count(), 0);

    if (doneExpected) {
        QVERIFY(process.isRunning());

        QVERIFY(process.waitForFinished());

        QVERIFY(!process.isRunning());
        QCOMPARE(doneSpy.count(), 1);

        if (gracefulQuit) {
            if (HostOsInfo::isWindowsHost())
                QSKIP(s_skipTerminateOnWindows);
            QCOMPARE(process.readAllRawStandardOutput(), s_leafProcessTerminated);
            QCOMPARE(process.exitStatus(), QProcess::NormalExit);
            QCOMPARE(process.exitCode(), s_crashCode);
        } else {
            QCOMPARE(process.readAllRawStandardOutput(), QByteArray());
            QCOMPARE(process.exitStatus(), QProcess::CrashExit);
            QVERIFY(process.exitCode() != s_crashCode);
        }
    } else {
        QVERIFY(!process.isRunning());
    }
}

void tst_Process::tarPipe()
{
    if (!FilePath::fromString("tar").searchInPath().isExecutableFile())
        QSKIP("This test uses \"tar\" command.");

    Process sourceProcess;
    Process targetProcess;

    targetProcess.setProcessMode(ProcessMode::Writer);

    QObject::connect(&sourceProcess, &Process::readyReadStandardOutput,
                     &targetProcess, [&sourceProcess, &targetProcess]() {
        targetProcess.writeRaw(sourceProcess.readAllRawStandardOutput());
    });

    QTemporaryDir sourceDir;
    QVERIFY(sourceDir.isValid());
    QTemporaryDir destinationDir;
    QVERIFY(destinationDir.isValid());

    const FilePath sourcePath = FilePath::fromString(sourceDir.path());
    const FilePath sourceArchive = sourcePath / "archive";
    QVERIFY(sourceArchive.createDir());
    const FilePath sourceFile = sourceArchive / "file1.txt";
    QVERIFY(sourceFile.writeFileContents("bla bla"));

    const FilePath destinationPath = FilePath::fromString(destinationDir.path());
    const FilePath destinationArchive = destinationPath / "archive";
    const FilePath destinationFile = destinationArchive / "file1.txt";

    QVERIFY(!destinationArchive.exists());
    QVERIFY(!destinationFile.exists());

    sourceProcess.setCommand({"tar", {"cvf", "-", "-C", sourcePath.nativePath(), "."}});
    targetProcess.setCommand({"tar", {"xvf", "-", "-C", destinationPath.nativePath()}});

    targetProcess.start();
    QVERIFY(targetProcess.waitForStarted());

    sourceProcess.start();
    QVERIFY(sourceProcess.waitForFinished());

    if (targetProcess.isRunning()) {
        targetProcess.closeWriteChannel();
        QVERIFY(targetProcess.waitForFinished(2s));
    }

    QCOMPARE(targetProcess.exitCode(), 0);
    QCOMPARE(targetProcess.result(), ProcessResult::FinishedWithSuccess);
    QVERIFY(destinationArchive.exists());
    QVERIFY(destinationFile.exists());
    QCOMPARE(sourceFile.fileSize(), destinationFile.fileSize());
}

void tst_Process::stdinToShell()
{
    //  proc.setCommand({"cmd.exe", {}});  - Piping into cmd.exe does not appear to work.
    if (HostOsInfo::isWindowsHost())
        QSKIP("Skipping env test on Windows");

    Process proc;
    proc.setCommand(CommandLine{"sh"});
    proc.setWriteData("echo hallo");
    proc.runBlocking();

    QString result = proc.readAllStandardOutput().trimmed();
    QCOMPARE(result, "hallo");
}

void tst_Process::eventLoopMode_data()
{
    QTest::addColumn<EventLoopMode>("eventLoopMode");

    QTest::newRow("EventLoopMode::On") << EventLoopMode::On;
    QTest::newRow("EventLoopMode::Off") << EventLoopMode::Off;
}

void tst_Process::eventLoopMode()
{
    QFETCH(EventLoopMode, eventLoopMode);

    {
        SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
        Process process;
        subConfig.setupSubProcess(&process);
        process.runBlocking(10s, eventLoopMode);
        QCOMPARE(process.result(), ProcessResult::FinishedWithSuccess);
    }

    {
        Process process;
        process.setCommand(
            CommandLine{"there_is_a_big_chance_that_executable_with_that_name_does_not_exists"});
        process.runBlocking(10s, eventLoopMode);
        QCOMPARE(process.result(), ProcessResult::StartFailed);
    }
}

QTEST_GUILESS_MAIN(tst_Process)

#include "tst_process.moc"
