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

#include "processtestapp/processtestapp.h"

#include <app/app_version.h>

#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/launcherinterface.h>
#include <utils/porting.h>
#include <utils/processinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/singleton.h>
#include <utils/stringutils.h>
#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QRegularExpression>
#include <QtTest>

#include <iostream>
#include <fstream>

using namespace Utils;

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

static std::atomic_int s_processCounter = 0;
static const bool s_removeSingletonsOnLastProcess = false;
// The use of this class (when s_removeSingletonsOnLastProcess = true) guarantees that if all
// instances of QtcProcess are destructed at some point (i.e. after each test function ends)
// we also run process reaper's and process launcher's destructors. Otherwise
// (when s_removeSingletonsOnLastProcess = true) this is a no-op wrapper around QtcProcess.
class TestProcess : public QtcProcess
{
public:
    class TestProcessDeleter : public QObject
    {
    public:
        TestProcessDeleter(QObject *parent) : QObject(parent) { s_processCounter.fetch_add(1); }
        ~TestProcessDeleter() {
            if ((s_processCounter.fetch_sub(1) == 1) && s_removeSingletonsOnLastProcess)
                Utils::Singleton::deleteAll();
        }
    };
    TestProcess() { new TestProcessDeleter(this); }
};

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
    void waitForStartedAndFinished();
    void notRunningAfterStartingNonExistingProgram();
    void channelForwarding_data();
    void channelForwarding();
    void mergedChannels_data();
    void mergedChannels();
    void killBlockingProcess_data();
    void killBlockingProcess();
    void flushFinishedWhileWaitingForReadyRead();
    void emitOneErrorOnCrash();
    void crashAfterOneSecond();
    void recursiveCrashingProcess();
    void recursiveBlockingProcess();

    void cleanupTestCase();

private:
    void iteratorEditsHelper(OsType osType);

    Environment envWindows;
    Environment envLinux;

    MacroMapExpander mxWin;
    MacroMapExpander mxUnix;
    QString homeStr;
    QString home;

    MessageHandler *msgHandler = nullptr;
};

void tst_QtcProcess::initTestCase()
{
    msgHandler = new MessageHandler;
    Utils::TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/"
                                                + Core::Constants::IDE_CASED_ID + "-XXXXXX");
    const QString libExecPath(qApp->applicationDirPath() + '/'
                              + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
    LauncherInterface::setPathToLauncher(libExecPath);
    SubProcessConfig::setPathToProcessTestApp(QLatin1String(PROCESS_TESTAPP));

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
    const int destroyCount = msgHandler->destroyCount();
    delete msgHandler;
    if (destroyCount)
        qDebug() << "Received" << destroyCount << "messages about destroying running QProcess!";
    QCOMPARE(destroyCount, 0);
}

Q_DECLARE_METATYPE(ProcessArgs::SplitError)
Q_DECLARE_METATYPE(Utils::OsType)
Q_DECLARE_METATYPE(Utils::ProcessResult)

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

    SubProcessConfig subConfig(ProcessTestApp::ExitCode::envVar(), QString::number(exitCode));
    {
        TestProcess process;
        subConfig.setupSubProcess(&process);
        process.start();
        const bool finished = process.waitForFinished();

        QVERIFY(finished);
        QCOMPARE(process.exitCode(), exitCode);
        QCOMPARE(process.exitCode() == 0, process.result() == ProcessResult::FinishedWithSuccess);
    }
    {
        TestProcess process;
        subConfig.setupSubProcess(&process);
        process.runBlocking();

        QCOMPARE(process.exitCode(), exitCode);
        QCOMPARE(process.exitCode() == 0, process.result() == ProcessResult::FinishedWithSuccess);
    }
}

void tst_QtcProcess::runBlockingStdOut_data()
{
    QTest::addColumn<bool>("withEndl");
    QTest::addColumn<int>("timeOutS");
    QTest::addColumn<ProcessResult>("expectedResult");

    // TerminatedAbnormally, since it didn't time out and callback stopped the process forcefully.
    QTest::newRow("Short timeout with end of line")
            << true << 2 << ProcessResult::TerminatedAbnormally;

    // Hang, since it times out, calls the callback handler and stops the process forcefully.
    QTest::newRow("Short timeout without end of line")
            << false << 2 << ProcessResult::Hang;

    // FinishedWithSuccess, since it doesn't time out, it finishes process normally,
    // calls the callback handler and tries to stop the process forcefully what is no-op
    // at this point in time since the process is already finished.
    QTest::newRow("Long timeout without end of line")
            << false << 20 << ProcessResult::FinishedWithSuccess;
}

void tst_QtcProcess::runBlockingStdOut()
{
    QFETCH(bool, withEndl);
    QFETCH(int, timeOutS);
    QFETCH(ProcessResult, expectedResult);

    SubProcessConfig subConfig(ProcessTestApp::RunBlockingStdOut::envVar(), withEndl ? "true" : "false");
    TestProcess process;
    subConfig.setupSubProcess(&process);

    process.setTimeoutS(timeOutS);
    bool readLastLine = false;
    process.setStdOutCallback([&readLastLine, &process](const QString &out) {
        if (out.startsWith(s_runBlockingStdOutSubProcessMagicWord)) {
            readLastLine = true;
            process.kill();
        }
    });
    process.runBlocking();

    // See also QTCREATORBUG-25667 for why it is a bad idea to use QtcProcess::runBlocking
    // with interactive cli tools.
    QCOMPARE(process.result(), expectedResult);
    QVERIFY2(readLastLine, "Last line was read.");
}

void tst_QtcProcess::lineCallback()
{
    SubProcessConfig subConfig(ProcessTestApp::LineCallback::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

    QStringList lines = QString(s_lineCallbackData).split('|');
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

void tst_QtcProcess::waitForStartedAndFinished()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

    process.start();
    QThread::msleep(1000); // long enough for process to finish
    QVERIFY(process.waitForStarted());
    QVERIFY(process.waitForFinished());
    QVERIFY(!process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);
}

void tst_QtcProcess::notRunningAfterStartingNonExistingProgram()
{
    TestProcess process;
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

// Since we want to test whether the process forwards its channels or not, we can't just create
// a process and start it, because in this case there is no way on how to check whether something
// went into our output channels or not.

// So we start two processes in chain instead. On the beginning the channelForwarding()
// test starts the ChannelForwarding::main() - this one will start another process
// StandardOutputAndErrorWriter::main() with forwarding options.
// The StandardOutputAndErrorWriter::main() is very simple - it just puts something to the output
// and the error channels. Then ChannelForwarding::main() either forwards these channels
// or not - we check it in the outer channelForwarding() test.

void tst_QtcProcess::channelForwarding_data()
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

void tst_QtcProcess::channelForwarding()
{
    QFETCH(QProcess::ProcessChannelMode, channelMode);
    QFETCH(bool, outputForwarded);
    QFETCH(bool, errorForwarded);

    SubProcessConfig subConfig(ProcessTestApp::ChannelForwarding::envVar(),
                               QString::number(int(channelMode)));
    TestProcess process;
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForFinished());

    const QByteArray output = process.readAllStandardOutput();
    const QByteArray error = process.readAllStandardError();

    QCOMPARE(output.contains(QByteArray(s_outputData)), outputForwarded);
    QCOMPARE(error.contains(QByteArray(s_errorData)), errorForwarded);
}

void tst_QtcProcess::mergedChannels_data()
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

void tst_QtcProcess::mergedChannels()
{
    QFETCH(QProcess::ProcessChannelMode, channelMode);
    QFETCH(bool, outputOnOutput);
    QFETCH(bool, outputOnError);
    QFETCH(bool, errorOnOutput);
    QFETCH(bool, errorOnError);

    SubProcessConfig subConfig(ProcessTestApp::StandardOutputAndErrorWriter::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

    process.setProcessChannelMode(channelMode);
    process.start();
    QVERIFY(process.waitForFinished());

    const QByteArray output = process.readAllStandardOutput();
    const QByteArray error = process.readAllStandardError();

    QCOMPARE(output.contains(QByteArray(s_outputData)), outputOnOutput);
    QCOMPARE(error.contains(QByteArray(s_outputData)), outputOnError);
    QCOMPARE(output.contains(QByteArray(s_errorData)), errorOnOutput);
    QCOMPARE(error.contains(QByteArray(s_errorData)), errorOnError);
}

void tst_QtcProcess::killBlockingProcess_data()
{
    QTest::addColumn<BlockType>("blockType");

    QTest::newRow("EndlessLoop") << BlockType::EndlessLoop;
    QTest::newRow("InfiniteSleep") << BlockType::InfiniteSleep;
    QTest::newRow("MutexDeadlock") << BlockType::MutexDeadlock;
    QTest::newRow("EventLoop") << BlockType::EventLoop;
}

void tst_QtcProcess::killBlockingProcess()
{
    QFETCH(BlockType, blockType);

    SubProcessConfig subConfig(ProcessTestApp::KillBlockingProcess::envVar(),
                               QString::number(int(blockType)));

    TestProcess process;
    subConfig.setupSubProcess(&process);
    process.start();
    QVERIFY(process.waitForStarted());
    QVERIFY(!process.waitForFinished(1000));
}

void tst_QtcProcess::flushFinishedWhileWaitingForReadyRead()
{
    SubProcessConfig subConfig(ProcessTestApp::SimpleTest::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

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
    QVERIFY(reply.contains(s_simpleTestData));
}

void tst_QtcProcess::emitOneErrorOnCrash()
{
    SubProcessConfig subConfig(ProcessTestApp::EmitOneErrorOnCrash::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

    int errorCount = 0;
    connect(&process, &QtcProcess::errorOccurred, this, [&errorCount] { ++errorCount; });
    process.start();
    QVERIFY(process.waitForStarted(1000));

    QEventLoop loop;
    connect(&process, &QtcProcess::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QCOMPARE(errorCount, 1);
    QCOMPARE(process.error(), QProcess::Crashed);
}

void tst_QtcProcess::crashAfterOneSecond()
{
    SubProcessConfig subConfig(ProcessTestApp::CrashAfterOneSecond::envVar(), {});
    TestProcess process;
    subConfig.setupSubProcess(&process);

    process.start();
    QVERIFY(process.waitForStarted(1000));
    QElapsedTimer timer;
    timer.start();
    // Please note that QProcess documentation says it should return false, but apparently
    // it doesn't (try running this test with QTC_USE_QPROCESS=)
    QVERIFY(process.waitForFinished(2000));
    QVERIFY(timer.elapsed() < 2000);
    QCOMPARE(process.state(), QProcess::NotRunning);
    QCOMPARE(process.error(), QProcess::Crashed);
}

void tst_QtcProcess::recursiveCrashingProcess()
{
    const int recursionDepth = 5; // must be at least 2
    SubProcessConfig subConfig(ProcessTestApp::RecursiveCrashingProcess::envVar(),
                               QString::number(recursionDepth));
    TestProcess process;
    subConfig.setupSubProcess(&process);
    process.start();
    QVERIFY(process.waitForStarted(1000));
    QVERIFY(process.waitForFinished(3000));
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

void tst_QtcProcess::recursiveBlockingProcess()
{
    if (HostOsInfo::isWindowsHost())
        QSKIP("Windows implementation of this test is lacking handling of WM_CLOSE message.");

    Singleton::deleteAll();
    QCOMPARE(runningTestProcessCount(), 0);
    const int recursionDepth = 5; // must be at least 2
    SubProcessConfig subConfig(ProcessTestApp::RecursiveBlockingProcess::envVar(),
                               QString::number(recursionDepth));
    {
        TestProcess process;
        subConfig.setupSubProcess(&process);
        process.start();
        QVERIFY(process.waitForStarted(1000));
        QVERIFY(process.waitForReadyRead(1000));
        QCOMPARE(process.readAllStandardOutput(), s_leafProcessStarted);
        QCOMPARE(runningTestProcessCount(), recursionDepth);
        QVERIFY(!process.waitForFinished(1000));
        process.terminate();
        QVERIFY(process.waitForReadyRead());
        QCOMPARE(process.readAllStandardOutput(), s_leafProcessTerminated);
        QVERIFY(process.waitForFinished());
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), s_crashCode);
    }
    Singleton::deleteAll();
    QCOMPARE(runningTestProcessCount(), 0);
}

QTEST_MAIN(tst_QtcProcess)

#include "tst_qtcprocess.moc"
