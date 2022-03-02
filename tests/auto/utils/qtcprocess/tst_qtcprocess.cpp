/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/launcherinterface.h>
#include <utils/porting.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/stringutils.h>

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QtTest>

#include <iostream>
#include <fstream>

#ifdef Q_OS_WIN
  #include <io.h>
  #include <fcntl.h>
#endif


using namespace Utils;

// Many tests in this file need to start a new subprocess with custom code.
// In order to simplify things we don't produce separate executables, but invoke
// the same test process recursively and prior to the execution we set one of the
// following environment variables:

const char kExitCodeSubProcessCode[] = "QTC_TST_QTCPROCESS_EXITCODE_CODE";
const char kRunBlockingStdOutSubProcessWithEndl[] = "QTC_TST_QTCPROCESS_RUNBLOCKINGSTDOUT_WITHENDL";
const char kLineCallback[] = "QTC_TST_QTCPROCESS_LINECALLBACK";
const char kTestProcess[] = "QTC_TST_TEST_PROCESS";
const char kForwardProcess[] = "QTC_TST_FORWARD_PROCESS";
const char kForwardSubProcess[] = "QTC_TST_FORWARD_SUB_PROCESS";
const char kBlockingProcess[] = "QTC_TST_BLOCKING_PROCESS";

// The variables above are meant to be different custom executables. Inside initTestCase()
// we are detecting if one of these variables is set and invoke directly a respective custom
// executable code, which always ends up with a call to exit(). In this case we need to stop
// the recursion, as from the test point of view we meant to execute only our custom code
// without further execution of the test itself.

const char testProcessData[] = "Test process successfully executed.";
const char forwardedOutputData[] = "This is the output message.";
const char forwardedErrorData[] = "This is the error message.";
const char runBlockingStdOutSubProcessMagicWord[] = "42";

// Expect ending lines detected at '|':
const char lineCallbackData[] =
       "This is the first line\r\n|"
       "Here comes the second one\r\n|"
       "And a line without LF\n|"
       "Rebasing (1/10)\r| <delay> Rebasing (2/10)\r| <delay> ...\r\n|"
       "And no end";

static void exitCodeSubProcessMain()
{
    const int exitCode = qEnvironmentVariableIntValue(kExitCodeSubProcessCode);
    std::cout << "Exiting with code:" << exitCode << std::endl;
    exit(exitCode);
}

static void blockingStdOutSubProcessMain()
{
    std::cout << "Wait for the Answer to the Ultimate Question of Life, "
                 "The Universe, and Everything..." << std::endl;
    QThread::msleep(300);
    std::cout << runBlockingStdOutSubProcessMagicWord << "...Now wait for the question...";
    if (qEnvironmentVariable(kRunBlockingStdOutSubProcessWithEndl) == "true")
        std::cout << std::endl;
    QThread::msleep(5000);
    exit(0);
}

static void lineCallbackMain()
{
#ifdef Q_OS_WIN
    // Prevent \r\n -> \r\r\n translation.
    setmode(fileno(stderr), O_BINARY);
#endif
    fprintf(stderr, "%s", QByteArray(lineCallbackData).replace('|', "").data());
    exit(0);
}

static void testProcessSubProcessMain()
{
    std::cout << testProcessData << std::endl;
    exit(0);
}

// Since we want to test whether the process forwards its channels or not, we can't just create
// a process and start it, because in this case there is no way on how to check whether something
// went into out output channels or not.

// So we start two processes in chain instead. On the beginning the processChannelForwarding()
// test starts the "testForwardProcessMain" - this one will start another process
// "testForwardSubProcessMain" with forwarding options. The "testForwardSubProcessMain"
// is very simple - it just puts something to the output and the error channels.
// Then "testForwardProcessMain" either forwards these channels or not - we check it in the outer
// processChannelForwarding() test.
static void testForwardProcessMain()
{
    Environment env = Environment::systemEnvironment();
    env.set(kForwardSubProcess, {});
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    QtcProcess process;
    const QProcess::ProcessChannelMode channelMode
            = QProcess::ProcessChannelMode(qEnvironmentVariableIntValue(kForwardProcess));
    process.setProcessChannelMode(channelMode);
    process.setCommand(command);
    process.setEnvironment(env);
    process.start();
    process.waitForFinished();
    exit(0);
}

static void testForwardSubProcessMain()
{
    std::cout << forwardedOutputData << std::endl;
    std::cerr << forwardedErrorData << std::endl;
    exit(0);
}

static void blockingProcessSubProcessMain()
{
    std::cout << "Blocking process successfully executed." << std::endl;
    while (true)
        ;
}

class MacroMapExpander : public AbstractMacroExpander {
public:
    virtual bool resolveMacro(const QString &name, QString *ret, QSet<AbstractMacroExpander*> &seen)
    {
        // loop prevention
        const int count = seen.count();
        seen.insert(this);
        if (seen.count() == count)
            return false;

        QHash<QString, QString>::const_iterator it = m_map.constFind(name);
        if (it != m_map.constEnd()) {
            *ret = it.value();
            return true;
        }
        return false;
    }
    void insert(const QString &key, const QString &value) { m_map.insert(key, value); }
private:
    QHash<QString, QString> m_map;
};

class tst_QtcProcess : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void splitArgs_data();
    void splitArgs();
    void prepareArgs_data();
    void prepareArgs();
    void prepareArgsEnv_data();
    void prepareArgsEnv();
    void expandMacros_data();
    void expandMacros();
    void iterations_data();
    void iterations();
    void iteratorEditsWindows();
    void iteratorEditsLinux();
    void exitCode_data();
    void exitCode();
    void runBlockingStdOut_data();
    void runBlockingStdOut();
    void lineCallback();
    void lineCallbackIntern();
    void waitForStartedAndFinished();
    void notRunningAfterStartingNonExistingProgram();
    void processChannelForwarding_data();
    void processChannelForwarding();
    void killBlockingProcess();
    void flushFinishedWhileWaitingForReadyRead();

    void cleanupTestCase();

private:
    void iteratorEditsHelper(OsType osType);

    Environment envWindows;
    Environment envLinux;

    MacroMapExpander mxWin;
    MacroMapExpander mxUnix;
    QString homeStr;
    QString home;
};

void tst_QtcProcess::initTestCase()
{
    Utils::LauncherInterface::setPathToLauncher(qApp->applicationDirPath() + '/'
                                                + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    if (qEnvironmentVariableIsSet(kExitCodeSubProcessCode))
        exitCodeSubProcessMain();
    if (qEnvironmentVariableIsSet(kRunBlockingStdOutSubProcessWithEndl))
        blockingStdOutSubProcessMain();
    if (qEnvironmentVariableIsSet(kLineCallback))
        lineCallbackMain();
    if (qEnvironmentVariableIsSet(kTestProcess))
        testProcessSubProcessMain();
    if (qEnvironmentVariableIsSet(kForwardSubProcess))
        testForwardSubProcessMain();
    else if (qEnvironmentVariableIsSet(kForwardProcess))
        testForwardProcessMain();
    if (qEnvironmentVariableIsSet(kBlockingProcess))
        blockingProcessSubProcessMain();

    homeStr = QLatin1String("@HOME@");
    home = QDir::homePath();

    QStringList env;
    env << "empty=" << "word=hi" << "words=hi ho" << "spacedwords= hi   ho sucker ";
    envWindows = Environment(env, OsTypeWindows);
    envLinux = Environment(env, OsTypeLinux);

    mxWin.insert("a", "hi");
    mxWin.insert("aa", "hi ho");

    mxWin.insert("b", "h\\i");
    mxWin.insert("c", "\\hi");
    mxWin.insert("d", "hi\\");
    mxWin.insert("ba", "h\\i ho");
    mxWin.insert("ca", "\\hi ho");
    mxWin.insert("da", "hi ho\\");

    mxWin.insert("e", "h\"i");
    mxWin.insert("f", "\"hi");
    mxWin.insert("g", "hi\"");

    mxWin.insert("h", "h\\\"i");
    mxWin.insert("i", "\\\"hi");
    mxWin.insert("j", "hi\\\"");

    mxWin.insert("k", "&special;");

    mxWin.insert("x", "\\");
    mxWin.insert("y", "\"");
    mxWin.insert("z", "");

    mxUnix.insert("a", "hi");
    mxUnix.insert("b", "hi ho");
    mxUnix.insert("c", "&special;");
    mxUnix.insert("d", "h\\i");
    mxUnix.insert("e", "h\"i");
    mxUnix.insert("f", "h'i");
    mxUnix.insert("z", "");
}

void tst_QtcProcess::cleanupTestCase()
{
    Utils::Singleton::deleteAll();
}

Q_DECLARE_METATYPE(ProcessArgs::SplitError)
Q_DECLARE_METATYPE(Utils::OsType)

void tst_QtcProcess::splitArgs_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<ProcessArgs::SplitError>("err");
    QTest::addColumn<Utils::OsType>("os");

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

void tst_QtcProcess::splitArgs()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(ProcessArgs::SplitError, err);
    QFETCH(Utils::OsType, os);

    ProcessArgs::SplitError outerr;
    QString outstr = ProcessArgs::joinArgs(ProcessArgs::splitArgs(in, os, false, &outerr), os);
    QCOMPARE(outerr, err);
    if (err == ProcessArgs::SplitOk)
        QCOMPARE(outstr, out);
}

void tst_QtcProcess::prepareArgs_data()
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

void tst_QtcProcess::prepareArgs()
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

void tst_QtcProcess::prepareArgsEnv_data()
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

void tst_QtcProcess::prepareArgsEnv()
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

void tst_QtcProcess::expandMacros_data()

{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");
    QTest::addColumn<OsType>("os");
    QChar sp(QLatin1Char(' '));

    static const struct {
        const char * const in;
        const char * const out;
        OsType os;
    } vals[] = {
        {"plain", 0, OsTypeWindows},
        {"%{a}", "hi", OsTypeWindows},
        {"%{aa}", "\"hi ho\"", OsTypeWindows},
        {"%{b}", "h\\i", OsTypeWindows},
        {"%{c}", "\\hi", OsTypeWindows},
        {"%{d}", "hi\\", OsTypeWindows},
        {"%{ba}", "\"h\\i ho\"", OsTypeWindows},
        {"%{ca}", "\"\\hi ho\"", OsTypeWindows},
        {"%{da}", "\"hi ho\\\\\"", OsTypeWindows}, // or "\"hi ho\"\\"
        {"%{e}", "\"h\"\\^\"\"i\"", OsTypeWindows},
        {"%{f}", "\"\"\\^\"\"hi\"", OsTypeWindows},
        {"%{g}", "\"hi\"\\^\"\"\"", OsTypeWindows},
        {"%{h}", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
        {"%{i}", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
        {"%{j}", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
        {"%{k}", "\"&special;\"", OsTypeWindows},
        {"%{x}", "\\", OsTypeWindows},
        {"%{y}", "\"\"\\^\"\"\"", OsTypeWindows},
        {"%{z}", "\"\"", OsTypeWindows},
        {"^%{z}%{z}", "^%{z}%{z}", OsTypeWindows}, // stupid user check

        {"quoted", 0, OsTypeWindows},
        {"\"%{a}\"", "\"hi\"", OsTypeWindows},
        {"\"%{aa}\"", "\"hi ho\"", OsTypeWindows},
        {"\"%{b}\"", "\"h\\i\"", OsTypeWindows},
        {"\"%{c}\"", "\"\\hi\"", OsTypeWindows},
        {"\"%{d}\"", "\"hi\\\\\"", OsTypeWindows},
        {"\"%{ba}\"", "\"h\\i ho\"", OsTypeWindows},
        {"\"%{ca}\"", "\"\\hi ho\"", OsTypeWindows},
        {"\"%{da}\"", "\"hi ho\\\\\"", OsTypeWindows},
        {"\"%{e}\"", "\"h\"\\^\"\"i\"", OsTypeWindows},
        {"\"%{f}\"", "\"\"\\^\"\"hi\"", OsTypeWindows},
        {"\"%{g}\"", "\"hi\"\\^\"\"\"", OsTypeWindows},
        {"\"%{h}\"", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
        {"\"%{i}\"", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
        {"\"%{j}\"", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
        {"\"%{k}\"", "\"&special;\"", OsTypeWindows},
        {"\"%{x}\"", "\"\\\\\"", OsTypeWindows},
        {"\"%{y}\"", "\"\"\\^\"\"\"", OsTypeWindows},
        {"\"%{z}\"", "\"\"", OsTypeWindows},

        {"leading bs", 0, OsTypeWindows},
        {"\\%{a}", "\\hi", OsTypeWindows},
        {"\\%{aa}", "\\\\\"hi ho\"", OsTypeWindows},
        {"\\%{b}", "\\h\\i", OsTypeWindows},
        {"\\%{c}", "\\\\hi", OsTypeWindows},
        {"\\%{d}", "\\hi\\", OsTypeWindows},
        {"\\%{ba}", "\\\\\"h\\i ho\"", OsTypeWindows},
        {"\\%{ca}", "\\\\\"\\hi ho\"", OsTypeWindows},
        {"\\%{da}", "\\\\\"hi ho\\\\\"", OsTypeWindows},
        {"\\%{e}", "\\\\\"h\"\\^\"\"i\"", OsTypeWindows},
        {"\\%{f}", "\\\\\"\"\\^\"\"hi\"", OsTypeWindows},
        {"\\%{g}", "\\\\\"hi\"\\^\"\"\"", OsTypeWindows},
        {"\\%{h}", "\\\\\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
        {"\\%{i}", "\\\\\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
        {"\\%{j}", "\\\\\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
        {"\\%{x}", "\\\\", OsTypeWindows},
        {"\\%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
        {"\\%{z}", "\\", OsTypeWindows},

        {"trailing bs", 0, OsTypeWindows},
        {"%{a}\\", "hi\\", OsTypeWindows},
        {"%{aa}\\", "\"hi ho\"\\", OsTypeWindows},
        {"%{b}\\", "h\\i\\", OsTypeWindows},
        {"%{c}\\", "\\hi\\", OsTypeWindows},
        {"%{d}\\", "hi\\\\", OsTypeWindows},
        {"%{ba}\\", "\"h\\i ho\"\\", OsTypeWindows},
        {"%{ca}\\", "\"\\hi ho\"\\", OsTypeWindows},
        {"%{da}\\", "\"hi ho\\\\\"\\", OsTypeWindows},
        {"%{e}\\", "\"h\"\\^\"\"i\"\\", OsTypeWindows},
        {"%{f}\\", "\"\"\\^\"\"hi\"\\", OsTypeWindows},
        {"%{g}\\", "\"hi\"\\^\"\"\"\\", OsTypeWindows},
        {"%{h}\\", "\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows},
        {"%{i}\\", "\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows},
        {"%{j}\\", "\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows},
        {"%{x}\\", "\\\\", OsTypeWindows},
        {"%{y}\\", "\"\"\\^\"\"\"\\", OsTypeWindows},
        {"%{z}\\", "\\", OsTypeWindows},

        {"bs-enclosed", 0, OsTypeWindows},
        {"\\%{a}\\", "\\hi\\", OsTypeWindows},
        {"\\%{aa}\\", "\\\\\"hi ho\"\\", OsTypeWindows},
        {"\\%{b}\\", "\\h\\i\\", OsTypeWindows},
        {"\\%{c}\\", "\\\\hi\\", OsTypeWindows},
        {"\\%{d}\\", "\\hi\\\\", OsTypeWindows},
        {"\\%{ba}\\", "\\\\\"h\\i ho\"\\", OsTypeWindows},
        {"\\%{ca}\\", "\\\\\"\\hi ho\"\\", OsTypeWindows},
        {"\\%{da}\\", "\\\\\"hi ho\\\\\"\\", OsTypeWindows},
        {"\\%{e}\\", "\\\\\"h\"\\^\"\"i\"\\", OsTypeWindows},
        {"\\%{f}\\", "\\\\\"\"\\^\"\"hi\"\\", OsTypeWindows},
        {"\\%{g}\\", "\\\\\"hi\"\\^\"\"\"\\", OsTypeWindows},
        {"\\%{h}\\", "\\\\\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows},
        {"\\%{i}\\", "\\\\\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows},
        {"\\%{j}\\", "\\\\\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows},
        {"\\%{x}\\", "\\\\\\", OsTypeWindows},
        {"\\%{y}\\", "\\\\\"\"\\^\"\"\"\\", OsTypeWindows},
        {"\\%{z}\\", "\\\\", OsTypeWindows},

        {"bs-enclosed and trailing literal quote", 0, OsTypeWindows},
        {"\\%{a}\\\\\\^\"", "\\hi\\\\\\^\"", OsTypeWindows},
        {"\\%{aa}\\\\\\^\"", "\\\\\"hi ho\"\\\\\\^\"", OsTypeWindows},
        {"\\%{b}\\\\\\^\"", "\\h\\i\\\\\\^\"", OsTypeWindows},
        {"\\%{c}\\\\\\^\"", "\\\\hi\\\\\\^\"", OsTypeWindows},
        {"\\%{d}\\\\\\^\"", "\\hi\\\\\\\\\\^\"", OsTypeWindows},
        {"\\%{ba}\\\\\\^\"", "\\\\\"h\\i ho\"\\\\\\^\"", OsTypeWindows},
        {"\\%{ca}\\\\\\^\"", "\\\\\"\\hi ho\"\\\\\\^\"", OsTypeWindows},
        {"\\%{da}\\\\\\^\"", "\\\\\"hi ho\\\\\"\\\\\\^\"", OsTypeWindows},
        {"\\%{e}\\\\\\^\"", "\\\\\"h\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows},
        {"\\%{f}\\\\\\^\"", "\\\\\"\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows},
        {"\\%{g}\\\\\\^\"", "\\\\\"hi\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
        {"\\%{h}\\\\\\^\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows},
        {"\\%{i}\\\\\\^\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows},
        {"\\%{j}\\\\\\^\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
        {"\\%{x}\\\\\\^\"", "\\\\\\\\\\\\\\^\"", OsTypeWindows},
        {"\\%{y}\\\\\\^\"", "\\\\\"\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
        {"\\%{z}\\\\\\^\"", "\\\\\\\\\\^\"", OsTypeWindows},

        {"bs-enclosed and trailing unclosed quote", 0, OsTypeWindows},
        {"\\%{a}\\\\\"", "\\hi\\\\\"", OsTypeWindows},
        {"\\%{aa}\\\\\"", "\\\\\"hi ho\"\\\\\"", OsTypeWindows},
        {"\\%{b}\\\\\"", "\\h\\i\\\\\"", OsTypeWindows},
        {"\\%{c}\\\\\"", "\\\\hi\\\\\"", OsTypeWindows},
        {"\\%{d}\\\\\"", "\\hi\\\\\\\\\"", OsTypeWindows},
        {"\\%{ba}\\\\\"", "\\\\\"h\\i ho\"\\\\\"", OsTypeWindows},
        {"\\%{ca}\\\\\"", "\\\\\"\\hi ho\"\\\\\"", OsTypeWindows},
        {"\\%{da}\\\\\"", "\\\\\"hi ho\\\\\"\\\\\"", OsTypeWindows},
        {"\\%{e}\\\\\"", "\\\\\"h\"\\^\"\"i\"\\\\\"", OsTypeWindows},
        {"\\%{f}\\\\\"", "\\\\\"\"\\^\"\"hi\"\\\\\"", OsTypeWindows},
        {"\\%{g}\\\\\"", "\\\\\"hi\"\\^\"\"\"\\\\\"", OsTypeWindows},
        {"\\%{h}\\\\\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\"", OsTypeWindows},
        {"\\%{i}\\\\\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\"", OsTypeWindows},
        {"\\%{j}\\\\\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\"", OsTypeWindows},
        {"\\%{x}\\\\\"", "\\\\\\\\\\\\\"", OsTypeWindows},
        {"\\%{y}\\\\\"", "\\\\\"\"\\^\"\"\"\\\\\"", OsTypeWindows},
        {"\\%{z}\\\\\"", "\\\\\\\\\"", OsTypeWindows},

        {"multi-var", 0, OsTypeWindows},
        {"%{x}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
        {"%{x}%{z}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
        {"%{x}%{z}%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
        {"%{x}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows},
        {"%{x}%{z}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows},
        {"%{x}%{z}\\^\"", "\\\\\\^\"", OsTypeWindows},
        {"%{x}\\%{z}", "\\\\", OsTypeWindows},
        {"%{x}%{z}\\%{z}", "\\\\", OsTypeWindows},
        {"%{x}%{z}\\", "\\\\", OsTypeWindows},
        {"%{aa}%{a}", "\"hi hohi\"", OsTypeWindows},
        {"%{aa}%{aa}", "\"hi hohi ho\"", OsTypeWindows},
        {"%{aa}:%{aa}", "\"hi ho\":\"hi ho\"", OsTypeWindows},
        {"hallo ^|%{aa}^|", "hallo ^|\"hi ho\"^|", OsTypeWindows},

        {"quoted multi-var", 0, OsTypeWindows},
        {"\"%{x}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
        {"\"%{x}%{z}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
        {"\"%{x}%{z}%{y}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
        {"\"%{x}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
        {"\"%{x}%{z}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
        {"\"%{x}%{z}\"^\"\"\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
        {"\"%{x}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows},
        {"\"%{x}%{z}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows},
        {"\"%{x}%{z}\\\\\"", "\"\\\\\\\\\"", OsTypeWindows},
        {"\"%{aa}%{a}\"", "\"hi hohi\"", OsTypeWindows},
        {"\"%{aa}%{aa}\"", "\"hi hohi ho\"", OsTypeWindows},
        {"\"%{aa}:%{aa}\"", "\"hi ho:hi ho\"", OsTypeWindows},

        {"plain", 0, OsTypeLinux},
        {"%{a}", "hi", OsTypeLinux},
        {"%{b}", "'hi ho'", OsTypeLinux},
        {"%{c}", "'&special;'", OsTypeLinux},
        {"%{d}", "'h\\i'", OsTypeLinux},
        {"%{e}", "'h\"i'", OsTypeLinux},
        {"%{f}", "'h'\\''i'", OsTypeLinux},
        {"%{z}", "''", OsTypeLinux},
        {"\\%{z}%{z}", "\\%{z}%{z}", OsTypeLinux}, // stupid user check

        {"single-quoted", 0, OsTypeLinux},
        {"'%{a}'", "'hi'", OsTypeLinux},
        {"'%{b}'", "'hi ho'", OsTypeLinux},
        {"'%{c}'", "'&special;'", OsTypeLinux},
        {"'%{d}'", "'h\\i'", OsTypeLinux},
        {"'%{e}'", "'h\"i'", OsTypeLinux},
        {"'%{f}'", "'h'\\''i'", OsTypeLinux},
        {"'%{z}'", "''", OsTypeLinux},

        {"double-quoted", 0, OsTypeLinux},
        {"\"%{a}\"", "\"hi\"", OsTypeLinux},
        {"\"%{b}\"", "\"hi ho\"", OsTypeLinux},
        {"\"%{c}\"", "\"&special;\"", OsTypeLinux},
        {"\"%{d}\"", "\"h\\\\i\"", OsTypeLinux},
        {"\"%{e}\"", "\"h\\\"i\"", OsTypeLinux},
        {"\"%{f}\"", "\"h'i\"", OsTypeLinux},
        {"\"%{z}\"", "\"\"", OsTypeLinux},

        {"complex", 0, OsTypeLinux},
        {"echo \"$(echo %{a})\"", "echo \"$(echo hi)\"", OsTypeLinux},
        {"echo \"$(echo %{b})\"", "echo \"$(echo 'hi ho')\"", OsTypeLinux},
        {"echo \"$(echo \"%{a}\")\"", "echo \"$(echo \"hi\")\"", OsTypeLinux},
        // These make no sense shell-wise, but they test expando nesting
        {"echo \"%{echo %{a}}\"", "echo \"%{echo hi}\"", OsTypeLinux},
        {"echo \"%{echo %{b}}\"", "echo \"%{echo hi ho}\"", OsTypeLinux},
        {"echo \"%{echo \"%{a}\"}\"", "echo \"%{echo \"hi\"}\"", OsTypeLinux },
    };

    const char *title = 0;
    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++) {
        if (!vals[i].out) {
            title = vals[i].in;
        } else {
            char buf[80];
            sprintf(buf, "%s: %s", title, vals[i].in);
            QTest::newRow(buf) << QString::fromLatin1(vals[i].in)
                               << QString::fromLatin1(vals[i].out)
                               << vals[i].os;
            sprintf(buf, "padded %s: %s", title, vals[i].in);
            QTest::newRow(buf) << QString(sp + QString::fromLatin1(vals[i].in) + sp)
                               << QString(sp + QString::fromLatin1(vals[i].out) + sp)
                               << vals[i].os;
        }
    }
}

void tst_QtcProcess::expandMacros()
{
    QFETCH(QString, in);
    QFETCH(QString, out);
    QFETCH(OsType, os);

    if (os == OsTypeWindows)
        ProcessArgs::expandMacros(&in, &mxWin, os);
    else
        ProcessArgs::expandMacros(&in, &mxUnix, os);
    QCOMPARE(in, out);
}

void tst_QtcProcess::iterations_data()
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

void tst_QtcProcess::iterations()
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

void tst_QtcProcess::iteratorEditsHelper(OsType osType)
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

void tst_QtcProcess::iteratorEditsWindows()
{
    iteratorEditsHelper(OsTypeWindows);
}

void tst_QtcProcess::iteratorEditsLinux()
{
    iteratorEditsHelper(OsTypeLinux);
}

void tst_QtcProcess::exitCode_data()
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

void tst_QtcProcess::exitCode()
{
    QFETCH(int, exitCode);

    Environment env = Environment::systemEnvironment();
    env.set(kExitCodeSubProcessCode, QString::number(exitCode));
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    {
        QtcProcess qtcP;
        qtcP.setCommand(command);
        qtcP.setEnvironment(env);
        qtcP.start();
        const bool finished = qtcP.waitForFinished();

        QVERIFY(finished);
        QCOMPARE(qtcP.exitCode(), exitCode);
        QCOMPARE(qtcP.exitCode() == 0, qtcP.result() == ProcessResult::FinishedWithSuccess);
    }
    {
        QtcProcess sP;
        sP.setCommand(command);
        sP.setEnvironment(env);
        sP.runBlocking();

        QCOMPARE(sP.exitCode(), exitCode);
        QCOMPARE(sP.exitCode() == 0, sP.result() == ProcessResult::FinishedWithSuccess);
    }
}

void tst_QtcProcess::runBlockingStdOut_data()
{
    QTest::addColumn<bool>("withEndl");
    QTest::addColumn<int>("timeOutS");

    QTest::newRow("Terminated stdout delivered instantly")
            << true
            << 2;
    QTest::newRow("Unterminated stdout lost: early timeout")
            << false
            << 2;
    QTest::newRow("Unterminated stdout lost: hanging")
            << false
            << 20;
}

void tst_QtcProcess::runBlockingStdOut()
{
    QFETCH(bool, withEndl);
    QFETCH(int, timeOutS);

    QtcProcess sp;
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    sp.setCommand(CommandLine(FilePath::fromString(binary), args));
    Environment env = Environment::systemEnvironment();
    env.set(kRunBlockingStdOutSubProcessWithEndl, withEndl ? "true" : "false");
    sp.setEnvironment(env);
    sp.setTimeoutS(timeOutS);
    bool readLastLine = false;
    sp.setStdOutCallback([&readLastLine, &sp](const QString &out) {
        if (out.startsWith(runBlockingStdOutSubProcessMagicWord)) {
            readLastLine = true;
            sp.kill();
        }
    });
    sp.runBlocking();

    // See also QTCREATORBUG-25667 for why it is a bad idea to use QtcProcess::runBlocking
    // with interactive cli tools.
    QEXPECT_FAIL("Unterminated stdout lost: early timeout", "", Continue);
    QVERIFY2(sp.result() != ProcessResult::Hang, "Process run did not time out.");
    QEXPECT_FAIL("Unterminated stdout lost: early timeout", "", Continue);
    QVERIFY2(readLastLine, "Last line was read.");
}

void tst_QtcProcess::lineCallback()
{
    QtcProcess process;
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    process.setCommand(CommandLine(FilePath::fromString(binary), args));
    Environment env = Environment::systemEnvironment();
    env.set(kLineCallback, "Yes");
    process.setEnvironment(env);
    QStringList lines = QString(lineCallbackData).split('|');
    int lineNumber = 0;
    process.setStdErrLineCallback([lines, &lineNumber](const QString &actual) {
        QString expected = lines.at(lineNumber++);
        expected.replace("\r\n", "\n");
        QCOMPARE(actual, expected);
    });
    process.start();
    process.waitForFinished();
    QCOMPARE(lineNumber, lines.size());
}

void tst_QtcProcess::lineCallbackIntern()
{
    QtcProcess process;
    QStringList lines = QString(lineCallbackData).split('|');
    int lineNumber = 0;
    process.setStdOutLineCallback([lines, &lineNumber](const QString &actual) {
        QString expected = lines.at(lineNumber++);
        expected.replace("\r\n", "\n");
        QCOMPARE(actual, expected);
    });
    process.beginFeed();
    process.feedStdOut(QByteArray(lineCallbackData).replace('|', ""));
    process.endFeed();
    QCOMPARE(lineNumber, lines.size());
}

void tst_QtcProcess::waitForStartedAndFinished()
{
    Environment env = Environment::systemEnvironment();
    env.set(kTestProcess, {});
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    QtcProcess process;
    process.setCommand(command);
    process.setEnvironment(env);
    process.start();
    QThread::msleep(1000); // long enough for process to finish
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished());
    QVERIFY(!process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
}

void tst_QtcProcess::notRunningAfterStartingNonExistingProgram()
{
    QtcProcess process;
    process.setCommand({ FilePath::fromString(
              "there_is_a_big_chance_that_executable_with_that_name_does_not_exists"), {} });

    int errorCount = 0;
    QObject::connect(&process, &QtcProcess::errorOccurred,
                     [&errorCount](QProcess::ProcessError error) {
        ++errorCount;
        QCOMPARE(error, QProcess::FailedToStart);
    });

    const int loopCount = 2;
    for (int i = 0; i < loopCount; ++i) {
        // Work on the same process instance on every iteration
        process.start();

        QElapsedTimer timer;
        timer.start();
        const int maxWaitTimeMs = 1000;

        QVERIFY(!process.waitForStarted(maxWaitTimeMs));
        QVERIFY(timer.elapsed() < maxWaitTimeMs); // shouldn't wait, should finish immediately
        QCOMPARE(process.state(), QProcess::NotRunning);
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.error(), QProcess::FailedToStart);
        QVERIFY(process.exitCode() != 0);
    }
}

void tst_QtcProcess::processChannelForwarding_data()
{
    QTest::addColumn<QProcess::ProcessChannelMode>("channelMode");
    QTest::addColumn<bool>("outputForwarded");
    QTest::addColumn<bool>("errorForwarded");

    QTest::newRow("SeparateChannels") << QProcess::SeparateChannels << false << false;
    QTest::newRow("ForwardedChannels") << QProcess::ForwardedChannels << true << true;
    QTest::newRow("ForwardedOutputChannel") << QProcess::ForwardedOutputChannel << true << false;
    QTest::newRow("ForwardedErrorChannel") << QProcess::ForwardedErrorChannel << false << true;
}

void tst_QtcProcess::processChannelForwarding()
{
    QFETCH(QProcess::ProcessChannelMode, channelMode);
    QFETCH(bool, outputForwarded);
    QFETCH(bool, errorForwarded);

    Environment env = Environment::systemEnvironment();
    env.set(kForwardProcess, QString::number(int(channelMode)));
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    QtcProcess process;
    process.setCommand(command);
    process.setEnvironment(env);
    process.start();
    QVERIFY(process.waitForFinished());

    const QByteArray output = process.readAllStandardOutput();
    const QByteArray error = process.readAllStandardError();

    QCOMPARE(output.contains(QByteArray(forwardedOutputData)), outputForwarded);
    QCOMPARE(error.contains(QByteArray(forwardedErrorData)), errorForwarded);
}

// This handler is inspired by the one used in qtbase/tests/auto/corelib/io/qfile/tst_qfile.cpp
class MessageHandler {
public:
    MessageHandler(QtMessageHandler messageHandler = handler)
    {
        ok = true;
        oldMessageHandler = qInstallMessageHandler(messageHandler);
    }

    ~MessageHandler()
    {
        qInstallMessageHandler(oldMessageHandler);
    }

    static bool testPassed()
    {
        return ok;
    }

protected:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
    {
        if (msg.startsWith("QProcess: Destroyed while process")
                && msg.endsWith("is still running.")) {
            ok = false;
        }
        // Defer to old message handler.
        if (oldMessageHandler)
            oldMessageHandler(type, context, msg);
    }

    static QtMessageHandler oldMessageHandler;
    static bool ok;
};

bool MessageHandler::ok = true;
QtMessageHandler MessageHandler::oldMessageHandler = 0;

void tst_QtcProcess::killBlockingProcess()
{
    Environment env = Environment::systemEnvironment();
    env.set(kBlockingProcess, {});
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    MessageHandler msgHandler;
    {
        QtcProcess process;
        process.setCommand(command);
        process.setEnvironment(env);
        process.start();
        QVERIFY(process.waitForStarted());
        QVERIFY(!process.waitForFinished(1000));
    }

    QVERIFY(msgHandler.testPassed());
}

void tst_QtcProcess::flushFinishedWhileWaitingForReadyRead()
{
    Environment env = Environment::systemEnvironment();
    env.set(kTestProcess, {});
    QStringList args = QCoreApplication::arguments();
    const QString binary = args.takeFirst();
    const CommandLine command(FilePath::fromString(binary), args);

    QtcProcess process;
    process.setCommand(command);
    process.setEnvironment(env);
    process.start();

    QVERIFY(process.waitForStarted());
    QCOMPARE(process.state(), QProcess::Running);

    QDeadlineTimer timer(1000);
    QByteArray reply;
    while (process.state() == QProcess::Running) {
        process.waitForReadyRead(500);
        reply += process.readAllStandardOutput();
        if (timer.hasExpired())
            break;
    }

    QCOMPARE(process.state(), QProcess::NotRunning);
    QVERIFY(!timer.hasExpired());
    QVERIFY(reply.contains(testProcessData));
}

QTEST_MAIN(tst_QtcProcess)

#include "tst_qtcprocess.moc"
