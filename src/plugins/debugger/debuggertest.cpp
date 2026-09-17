// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "debuggertest.h"

#include "cdb/cdbengine.h"
#include "cdb/cdbimpl.h"
#include "cdb/cdbparsehelpers.h"
#include "debuggeractions.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerengineinterface.h"
#include "breakhandler.h"
#include "genericdebuggerengine.h"
#include "debuggeritem.h"
#include "debuggerruncontrol.h"
#include "debuggersourcepathmappingwidget.h"
#include "enginemanager.h"
#include "gdb/gdbengine.h"
#include "registerhandler.h"
#include "commonoptionspage.h"
#include "stackhandler.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/abi.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>

#include <QTest>
#include <QVersionNumber>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTestEventLoop>

#include <memory>

//#define WITH_BENCHMARK
#ifdef WITH_BENCHMARK
#include <valgrind/callgrind.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;
#endif // WITH_TESTS

namespace Debugger::Internal {
static bool s_testRun = false;
bool isTestRun() { return s_testRun; }
} // Debugger::Internal

#ifdef WITH_TESTS
namespace Debugger::Internal {

class DebuggerUnitTests : public QObject
{
    Q_OBJECT

public:
    DebuggerUnitTests() = default;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void testDebuggerMatching_data();
    void testDebuggerMatching();

    void testBenchmark();
    void testStateMachine();
    void testGdbDapEngineRunsASession();

    void testRegisterValue_data();
    void testRegisterValue();

    void testInferiorStartData();
    void testCdbImplStartData();
    void testCdbImplCommandLine();
    void testCdbImplArtificialThreadStop();
    void testCdbImplResolvedBreakpointUpdates();
    void testCdbImplBreakpointStopMessages();
    void testCdbImplStepIntoLanding();
    void testCdbImplScriptMessages();
    void testCdbImplBreakpointInsertCommand();
    void testCdbImplBreakpointModuleScope();
    void testCdbImplDisassemblyRange();
    void testCdbImplNormalizedSourceFileName();
    void testCdbImplModulesTree();
    void testCdbImplRegistersTree();
    void testCdbImplSetParameterArguments();
    void testMapsAnEmptyFileNameToNothing();
    void testCdbSourcePathMapping();
    void testCdbBreakpointFileName();

    void testQtBuildSourceRoots_data();
    void testQtBuildSourceRoots();

    void testDebugInfoDirectory();
    void testDebugInfoFile();
    void testMergePlatformQtPath();
    void testNormalizedSourcePathPrefix();

    void testStepsIntoACalledFunction();
    void testStepsOverACallWithoutEnteringIt();
    void testStepsOutOfACalledFunction();
    void testScratchEditorAdoptsSavedName();
    void testBreakpointUpdateAnnouncesItIsProceeding();
    void testInterpreterBreakpointStaysEnabled();
    void testNamespaceFromQObjectRtti_data();
    void testNamespaceFromQObjectRtti();

    void testTerminateMessage();

private:
    CppEditor::Tests::TemporaryCopiedDir *m_tmpDir = nullptr;
};

void DebuggerUnitTests::initTestCase()
{
//    const QList<Kit *> allKits = KitManager::kits();
//    if (allKits.count() != 1)
//        QSKIP("This test requires exactly one kit to be present");
//    const Toolchain * const toolchain = ToolchainKitAspect::toolchain(allKits.first());
//    if (!toolchain)
//        QSKIP("This test requires that there is a kit with a toolchain.");
//    bool hasClangExecutable;
//    clangExecutableFromSettings(toolchain->typeId(), &hasClangExecutable);
//    if (!hasClangExecutable)
//        QSKIP("No clang suitable for analyzing found");

    s_testRun = true;
    m_tmpDir = new CppEditor::Tests::TemporaryCopiedDir(":/debugger/unit-tests");
    QVERIFY(m_tmpDir->isValid());
}

void DebuggerUnitTests::cleanupTestCase()
{
    delete m_tmpDir;
}

void DebuggerUnitTests::testStateMachine()
{
    FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile));

    QEventLoop loop;
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            &loop, &QEventLoop::quit);
    BuildManager::buildProjectWithDependencies(ProjectManager::startupProject());
    loop.exec();

    const QScopeGuard cleanup([] { EditorManager::closeAllEditors(false); });

    RunConfiguration *rc = activeRunConfigForActiveProject();
    QVERIFY(rc);

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->copyDataFromRunConfiguration(rc);

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferior(rc->runnable());
    rp.setTestCase(TestNoBoundsOfCurrentFunction);

    connect(runControl, &RunControl::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();

    // The debuggee faults on purpose and is held there, so the session has to
    // be ended for it to report the stop.
    QTRY_VERIFY_WITH_TIMEOUT(runControl->isRunning(), 30000);
    runControl->initiateStop();
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

// The DAP engines are reachable only through a run mode, so whether one still
// works is answered by having it debug something. gdb speaks the protocol
// itself, which is what its DAP engine is for, so nothing else is needed.
void DebuggerUnitTests::testGdbDapEngineRunsASession()
{
    FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");

    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile));

    QEventLoop loop;
    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            &loop, &QEventLoop::quit);
    BuildManager::buildProjectWithDependencies(ProjectManager::startupProject());
    loop.exec();

    const QScopeGuard cleanup([] { EditorManager::closeAllEditors(false); });

    RunConfiguration *rc = activeRunConfigForActiveProject();
    QVERIFY(rc);

    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->copyDataFromRunConfiguration(rc);

    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setInferior(rc->runnable());
    // gdb only grew its DAP mode along the way, and the engine refuses an older
    // one rather than talking to something that will not answer.
    if (QVersionNumber::fromString(rp.version()) < QVersionNumber(14, 0, 50))
        QSKIP("The debugger of this kit is too old for its DAP mode.");
    // What sends the session to the DAP engine instead of the native one.
    rp.setCppEngineType(GdbDapEngineType);

    connect(runControl, &RunControl::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);

    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();

    // The debuggee faults on purpose, so a session that got through to the
    // adapter ends up holding it there. Nothing short of the whole exchange -
    // the adapter started, the initialize answered, the launch taken and the
    // stop reported - arrives at a stopped inferior.
    const auto dapEngine = [] {
        for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
            if (candidate && candidate->objectName() == "GdbDapEngine")
                return candidate;
        }
        return QPointer<DebuggerEngine>();
    };
    // EngineManager holds the whole application's engines, and the test before
    // this one leaves its own behind, so pick by name.
    QTRY_VERIFY_WITH_TIMEOUT(dapEngine(), 10000);
    const QPointer<DebuggerEngine> engine = dapEngine();
    QTRY_VERIFY_WITH_TIMEOUT(engine && engine->state() == InferiorStopOk, 30000);

    runControl->initiateStop();
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

enum FakeEnum { FakeDebuggerCommonSettingsId };

void DebuggerUnitTests::testBenchmark()
{
#ifdef WITH_BENCHMARK
    CALLGRIND_START_INSTRUMENTATION;
    volatile Id id1 = Id(DEBUGGER_COMMON_SETTINGS_ID);
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;

    CALLGRIND_START_INSTRUMENTATION;
    volatile FakeEnum id2 = FakeDebuggerCommonSettingsId;
    CALLGRIND_STOP_INSTRUMENTATION;
    CALLGRIND_DUMP_STATS;
#endif
}

void DebuggerUnitTests::testDebuggerMatching_data()
{
    QTest::addColumn<QStringList>("debugger");
    QTest::addColumn<QString>("target");
    QTest::addColumn<int>("result");

    QTest::newRow("Invalid data")
            << QStringList()
            << QString()
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid debugger")
            << QStringList()
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Invalid target")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString()
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Fuzzy match 1")
            << QStringList("unknown-unknown-unknown-unknown-0bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?
    QTest::newRow("Fuzzy match 2")
            << QStringList("unknown-unknown-unknown-unknown-0bit")
            << QString::fromLatin1("arm-windows-msys-pe-64bit")
            << int(DebuggerItem::MatchesWell); // Is this the expected behavior?

    QTest::newRow("Architecture mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("arm-linux-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("OS mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-macosx-generic-elf-32bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Format mismatch")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-linux-generic-pe-32bit")
            << int(DebuggerItem::DoesNotMatch);

    QTest::newRow("Linux perfect match")
            << QStringList("x86-linux-generic-elf-32bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Linux match")
            << QStringList("x86-linux-generic-elf-64bit")
            << QString::fromLatin1("x86-linux-generic-elf-32bit")
            << int(DebuggerItem::MatchesSomewhat);

    QTest::newRow("Windows perfect match 1")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows perfect match 2")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2012-pe-64bit")
            << int(DebuggerItem::MatchesWell);
    QTest::newRow("Windows match 1")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows match 2")
            << QStringList("x86-windows-msvc2013-pe-64bit")
            << QString::fromLatin1("x86-windows-msvc2012-pe-32bit")
            << int(DebuggerItem::MatchesSomewhat);
    QTest::newRow("Windows mismatch on word size")
            << QStringList("x86-windows-msvc2013-pe-32bit")
            << QString::fromLatin1("x86-windows-msvc2013-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 1")
            << QStringList("x86-windows-msvc2013-pe-32bit")
            << QString::fromLatin1("x86-windows-msys-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
    QTest::newRow("Windows mismatch on osflavor 2")
            << QStringList("x86-windows-msys-pe-32bit")
            << QString::fromLatin1("x86-windows-msvc2010-pe-64bit")
            << int(DebuggerItem::DoesNotMatch);
}

void DebuggerUnitTests::testDebuggerMatching()
{
    QFETCH(QStringList, debugger);
    QFETCH(QString, target);
    QFETCH(int, result);

    auto expectedLevel = static_cast<DebuggerItem::MatchLevel>(result);

    Abis debuggerAbis;
    for (const QString &abi : std::as_const(debugger))
        debuggerAbis << Abi::fromString(abi);

    DebuggerItem item;
    item.setAbis(debuggerAbis);

    DebuggerItem::MatchLevel level = item.matchTarget(Abi::fromString(target));
    if (level == DebuggerItem::MatchesPerfectly)
        level = DebuggerItem::MatchesWell;

    QCOMPARE(expectedLevel, level);
}

void DebuggerUnitTests::testRegisterValue_data()
{
    // Lowercase hex of the register value, no "0x"; its length defines the size.
    QTest::addColumn<QString>("hexValue");

    QTest::newRow("8bit") << "ab";
    QTest::newRow("32bit") << "89abcdef";
    QTest::newRow("64bit") << "0123456789abcdef";
    QTest::newRow("64bit-leading-zero") << "00000000deadbeef";
    QTest::newRow("128bit") << "fedcba98765432100123456789abcdef";
    QTest::newRow("128bit-high-only") << "ffffffffffffffff0000000000000000";
    // 256-bit (e.g. AVX YMM): used to show up as 0 because the upper half was dropped.
    QTest::newRow("256bit")
        << "99aabbccddeeff001122334455667788fedcba98765432100123456789abcdef";
    QTest::newRow("256bit-high-only")
        << "abcdef01234567899876543210fedcba00000000000000000000000000000000";
    QTest::newRow("256bit-all-f") << QString(64, 'f');
}

void DebuggerUnitTests::testRegisterValue()
{
    QFETCH(QString, hexValue);
    const int size = int(hexValue.size()) / 2; // bytes

    RegisterValue value;
    value.fromString("0x" + hexValue, HexadecimalFormat);

    // Round-trips through the (up to 256-bit wide) representation. On the old
    // 128-bit-only implementation the upper half of a 256-bit value was lost.
    QCOMPARE(value.toString(IntegerRegister, size, HexadecimalFormat), hexValue);

    // Decimal formatting goes through the synthesized 128-bit division.
    RegisterValue twoPow64;
    twoPow64.fromString("0x10000000000000000", HexadecimalFormat);
    QCOMPARE(twoPow64.toString(IntegerRegister, 16, DecimalFormat).trimmed(),
             QString("18446744073709551616"));
}

void DebuggerUnitTests::testInferiorStartData()
{
    // Attaching to a local process: the pid, not the run data, is what starts it.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToLocalProcess);
        rp.setInferiorExecutable("/usr/bin/tst_inferior");
        rp.setAttachPid(ProcessHandle(4711));

        const InferiorStartData data = inferiorStartData(rp);
        const auto *attachData = std::get_if<AttachToProcessData>(&data);
        QVERIFY(attachData);
        QCOMPARE(attachData->pid.pid(), 4711);
    }

    // A core file has no symbols of its own; the binary provides them.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToCore);
        rp.setCoreFilePath("/tmp/core.4711");
        rp.setInferiorExecutable("/usr/bin/tst_inferior");

        const InferiorStartData data = inferiorStartData(rp);
        const auto *coreData = std::get_if<AttachToCoreData>(&data);
        QVERIFY(coreData);
        QCOMPARE(coreData->coreFile, FilePath("/tmp/core.4711"));
        QCOMPARE(coreData->executable, FilePath("/usr/bin/tst_inferior"));
    }

    // The channel is passed to "target remote", which does not take the URL
    // form the debug channel is handed over as, nor a bare IPv6 host.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("tcp://192.168.1.1:1234");
        rp.setSymbolFile("/usr/bin/tst_inferior");

        const InferiorStartData data = inferiorStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->channel, QString("tcp:192.168.1.1:1234"));
        QCOMPARE(remoteData->symbolFile, FilePath("/usr/bin/tst_inferior"));
        QVERIFY(!remoteData->attachPid.isValid());
        QVERIFY(remoteData->remoteExecutable.isEmpty());
    }
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("fe80::1:1234");

        const InferiorStartData data = inferiorStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->channel, QString("tcp:[fe80::1]:1234"));
    }
    // A pipe channel, as vgdb uses, must be left alone.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("| vgdb --pid=4711");

        const InferiorStartData data = inferiorStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->channel, QString("| vgdb --pid=4711"));
    }

    // With "target extended-remote" gdb starts the inferior itself, so it
    // needs its path on the device - unless there is a pid to attach to.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("192.168.1.1:1234");
        rp.setUseExtendedRemote(true);
        rp.setInferiorExecutable("/data/tst_inferior");

        const InferiorStartData data = inferiorStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->remoteExecutable, FilePath("/data/tst_inferior"));
    }
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("192.168.1.1:1234");
        rp.setUseExtendedRemote(true);
        rp.setInferiorExecutable("/data/tst_inferior");
        rp.setAttachPid(ProcessHandle(4711));

        const InferiorStartData data = inferiorStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->attachPid.pid(), 4711);
        QVERIFY(remoteData->remoteExecutable.isEmpty());
    }
}

void DebuggerUnitTests::testCdbImplStartData()
{
    // A local cdb takes the extension shipping next to Qt Creator.
    {
        DebuggerRunParameters rp;
        rp.setInferiorExecutable("C:/build/tst_inferior.exe");

        const CdbImplStartData data = cdbImplStartData(rp);
        QCOMPARE(data.extensionFileName, QString("qtcreatorcdbext.dll"));
        QVERIFY(data.extensionDir.fileName().startsWith("qtcreatorcdbext"));
    }

    // A cdb on a device takes the one on the device, from the subdirectory matching
    // its architecture. Nothing of the cdb binary is readable here, so the default
    // 64 bit Intel applies.
    {
        DebuggerRunParameters rp;
        rp.setDebugger({CommandLine(FilePath::fromParts(u"ssh", u"somehost", u"C:/dbg/cdb.exe"))});
        rp.setInferiorExecutable("C:/build/tst_inferior.exe");
        rp.setCdbExtensionPath(FilePath::fromParts(u"ssh", u"somehost", u"C:/qtc/ext"));

        const CdbImplStartData data = cdbImplStartData(rp);
        QCOMPARE(data.extensionFileName, QString("qtcreatorcdbext.dll"));
        QCOMPARE(data.extensionDir,
                 FilePath::fromParts(u"ssh", u"somehost", u"C:/qtc/ext/qtcreatorcdbext64"));
    }

    // Without a configured extension path there is nothing to load on the device.
    {
        DebuggerRunParameters rp;
        rp.setDebugger({CommandLine(FilePath::fromParts(u"ssh", u"somehost", u"C:/dbg/cdb.exe"))});
        rp.setInferiorExecutable("C:/build/tst_inferior.exe");

        QVERIFY(cdbImplStartData(rp).extensionFileName.isEmpty());
    }

    // Running in a terminal, the stub has already created the program, suspended,
    // so there is only something to attach to.
    {
        DebuggerRunParameters rp;
        rp.setInferiorExecutable("C:/build/tst_inferior.exe");
        rp.setUseTerminal(true);
        rp.setApplicationPid(4711);
        rp.setApplicationMainThreadId(4712);

        const CdbImplStartData data = cdbImplStartData(rp);
        const auto *stubData = std::get_if<AttachToTerminalStubData>(&data.inferiorStartData);
        QVERIFY(stubData);
        QCOMPARE(stubData->pid.pid(), 4711);
        QCOMPARE(stubData->mainThreadId, 4712);
        QCOMPARE(stubData->executable, FilePath("C:/build/tst_inferior.exe"));
    }

    // cdb speaks its own "-remote" syntax, so the channel is passed on unchanged.
    {
        DebuggerRunParameters rp;
        rp.setStartMode(AttachToRemoteServer);
        rp.setRemoteChannel("tcp:port=1234,server=192.168.1.1");
        rp.setSymbolFile("C:/build/tst_inferior.exe");
        rp.setAttachPid(ProcessHandle(4711));

        const CdbImplStartData data = cdbImplStartData(rp);
        const auto *remoteData = std::get_if<AttachToRemoteServerData>(&data.inferiorStartData);
        QVERIFY(remoteData);
        QCOMPARE(remoteData->channel, QString("tcp:port=1234,server=192.168.1.1"));
        QCOMPARE(remoteData->symbolFile, FilePath("C:/build/tst_inferior.exe"));
        QCOMPARE(remoteData->attachPid.pid(), 4711);
    }
}

void DebuggerUnitTests::testCdbImplCommandLine()
{
    QTemporaryDir tmpDir;
    QVERIFY(tmpDir.isValid());
    const FilePath root = FilePath::fromString(tmpDir.path());
    const FilePath extensionDir = root / "qtcreatorcdbext64";
    QVERIFY(extensionDir.ensureWritableDir());
    const FilePath extension = extensionDir / "qtcreatorcdbext.dll";
    QVERIFY(extension.writeFileContents("MZ"));
    const FilePath inferiorDir = root / "build";
    QVERIFY(inferiorDir.ensureWritableDir());

    CdbImplStartData startData;
    startData.debuggerRunData.command = CommandLine(root / "cdb.exe");
    startData.extensionDir = extensionDir;
    startData.extensionFileName = extension.fileName();
    ProcessRunData inferior;
    inferior.command = CommandLine(inferiorDir / "tst_inferior.exe");
    inferior.workingDirectory = inferiorDir;
    startData.inferiorStartData = inferior;

    const QString idleCommand(".idle_cmd !qtcreatorcdbext.idle");

    // A local cdb finds the extension by name, through _NT_DEBUGGER_EXTENSION_PATH.
    {
        CdbImpl cdb(startData);
        QVERIFY(cdb.setupProcess());
        const QStringList args = cdb.m_cdbProc.commandLine().splitArguments();
        QVERIFY(args.contains("-aqtcreatorcdbext.dll"));
        QVERIFY(!args.contains("-cf"));
        QCOMPARE(args.value(args.indexOf("-c") + 1), idleCommand);
        QVERIFY(cdb.m_initScriptFile.isEmpty());
    }

    // A cdb on a device gets no environment, so the extension is loaded by its
    // absolute path, from a startup script staged next to the inferior.
    {
        CdbImplStartData deviceData = startData;
        deviceData.debuggerRunData.command
            = CommandLine(FilePath::fromParts(u"ssh", u"somehost", u"C:/dbg/cdb.exe"));

        CdbImpl cdb(deviceData);
        QVERIFY(cdb.setupProcess());
        const QStringList args = cdb.m_cdbProc.commandLine().splitArguments();
        QVERIFY(!args.contains("-aqtcreatorcdbext.dll"));
        QVERIFY(!args.contains("-c"));
        const int scriptIndex = args.indexOf("-cf") + 1;
        QVERIFY(scriptIndex > 0);
        QCOMPARE(args.at(scriptIndex), cdb.m_initScriptFile.nativePath());
        QCOMPARE(cdb.m_initScriptFile.parentDir(), inferiorDir);
        const QByteArray script = cdb.m_initScriptFile.fileContents().value_or(QByteArray());
        QCOMPARE(QString::fromLocal8Bit(script),
                 ".load " + extension.nativePath() + '\n' + idleCommand + '\n');
    }

    // Attaching to a remote server: "-remote" has to come first, the session loads
    // the extension itself once it stands, and the symbols come from the local copy
    // of the binary running on the target.
    {
        CdbImplStartData remoteData = startData;
        remoteData.inferiorStartData = AttachToRemoteServerData{
            "tcp:port=1234,server=192.168.1.1", inferiorDir / "tst_inferior.exe", {}, {}};

        CdbImpl cdb(remoteData);
        QVERIFY(cdb.setupProcess());
        const QStringList args = cdb.m_cdbProc.commandLine().splitArguments();
        QCOMPARE(args.value(0), QString("-remote"));
        QCOMPARE(args.value(1), QString("tcp:port=1234,server=192.168.1.1"));
        QVERIFY(!args.contains("-aqtcreatorcdbext.dll"));
        QVERIFY(!args.contains("-cf"));
        const QStringList symbolPaths = args.value(args.indexOf("-y") + 1).split(';');
        QVERIFY(symbolPaths.contains(inferiorDir.nativePath()));
    }

    // Attaching to what the stub suspended: "-pr" lets it run, "-pb" keeps cdb from
    // breaking into it, and the console the stub opened is the one it keeps.
    {
        CdbImplStartData stubData = startData;
        stubData.useTerminal = true;
        stubData.inferiorStartData
            = AttachToTerminalStubData{ProcessHandle(4711), 4712, inferiorDir / "tst_inferior.exe"};

        CdbImpl cdb(stubData);
        QVERIFY(cdb.setupData().startModes.testFlag(DebuggerStartModeFlag::AttachToTerminalStub));
        QVERIFY(cdb.setupProcess());
        const QStringList args = cdb.m_cdbProc.commandLine().splitArguments();
        QCOMPARE(args.value(args.indexOf("-p") + 1), QString("4711"));
        QVERIFY(args.contains("-pr"));
        QVERIFY(args.contains("-pb"));
        QVERIFY(!args.contains("-2"));
        const QStringList symbolPaths = args.value(args.indexOf("-y") + 1).split(';');
        QVERIFY(symbolPaths.contains(inferiorDir.nativePath()));
    }
}

void DebuggerUnitTests::testCdbImplArtificialThreadStop()
{
    const auto stopData = [](const QString &contents) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        GdbMi data;
        data.fromString('{' + contents + '}', decoder);
        return data;
    };

    // An interrupt request: the break runs in a thread Windows created for it.
    QVERIFY(stoppedInArtificialThread(stopData(
        R"(reason="exception",exceptionCode="2147483651",)"
        R"(exceptionFunction="ntdll!DbgBreakPoint")")));

    // Ctrl-C in the console the program runs in, same mechanism.
    QVERIFY(stoppedInArtificialThread(stopData(
        R"(reason="exception",exceptionCode="1073807365",)"
        R"(exceptionFunction="kernel32!CtrlRoutine")")));

    // A breakpoint the user set is in the user's own thread.
    QVERIFY(!stoppedInArtificialThread(stopData(R"(reason="breakpoint")")));

    // So is a crash, even though that too arrives as an exception.
    QVERIFY(!stoppedInArtificialThread(stopData(
        R"(reason="exception",exceptionCode="3221225477",)"
        R"(exceptionFunction="tst_inferior!crash")")));

    // A break the program hit by calling DebugBreak() itself is in its thread.
    QVERIFY(!stoppedInArtificialThread(stopData(
        R"(reason="exception",exceptionCode="2147483651",)"
        R"(exceptionFunction="tst_inferior!main")")));
}

void DebuggerUnitTests::testCdbImplResolvedBreakpointUpdates()
{
    const auto reply = [](const QString &contents) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        GdbMi data;
        data.fromString('[' + contents + ']', decoder);
        return data;
    };
    const QList<QPair<QString, QString>> noMapping;
    const QHash<QString, QString> noConditions;

    // The module holding the breakpoint has been loaded, so cdb now knows where
    // it sits. That is what the view is missing.
    QSet<QString> wanted{"3"};
    GdbMi updates = resolvedBreakpointUpdates(
        reply(R"({number="0",id="3",deferred="false",enabled="true",)"
              R"(address="0x7ff61f3a1020",module="tst_inferior",)"
              R"(srcfile="C:/src/main.cpp",srcline="42"})"),
        &wanted, noMapping, noConditions);
    QCOMPARE(updates.childCount(), 1);
    QCOMPARE(updates.childAt(0)["number"].data(), QString("3"));
    QCOMPARE(updates.childAt(0)["addr"].data(), QString("0x7ff61f3a1020"));
    QCOMPARE(updates.childAt(0)["module"].data(), QString("tst_inferior"));
    QCOMPARE(updates.childAt(0)["line"].data(), QString("42"));
    QCOMPARE(updates.childAt(0)["enabled"].data(), QString("y"));
    // Answered, so there is nothing left to ask about at the next stop.
    QVERIFY(wanted.isEmpty());

    // Still deferred: the module is not loaded yet, ask again later.
    wanted = {"3"};
    updates = resolvedBreakpointUpdates(
        reply(R"({number="0",id="3",deferred="true",enabled="true"})"),
        &wanted, noMapping, noConditions);
    QCOMPARE(updates.childCount(), 0);
    QVERIFY(wanted.contains("3"));

    // A breakpoint that already has its location is not one we asked about.
    wanted = {"3"};
    updates = resolvedBreakpointUpdates(
        reply(R"({number="0",id="7",deferred="false",enabled="true",)"
              R"(address="0x7ff61f3a1020"})"),
        &wanted, noMapping, noConditions);
    QCOMPARE(updates.childCount(), 0);
    QVERIFY(wanted.contains("3"));

    // updateFromGdbOutput() takes the update for the whole state, so a condition
    // has to be repeated or it is lost.
    wanted = {"3"};
    updates = resolvedBreakpointUpdates(
        reply(R"({number="0",id="3",deferred="false",enabled="false",)"
              R"(address="0x7ff61f3a1020"})"),
        &wanted, noMapping, {{"3", "i == 5"}});
    QCOMPARE(updates.childCount(), 1);
    QCOMPARE(updates.childAt(0)["cond"].data(), QString("i == 5"));
    QCOMPARE(updates.childAt(0)["enabled"].data(), QString("n"));

    // What the pdb records is where the sources were when the inferior was built.
    wanted = {"3"};
    updates = resolvedBreakpointUpdates(
        reply(R"({number="0",id="3",deferred="false",enabled="true",)"
              R"(address="0x7ff61f3a1020",srcfile="C:/build/src/main.cpp",srcline="42"})"),
        &wanted, {{"C:/build", "C:/work"}}, noConditions);
    QCOMPARE(updates.childCount(), 1);
    QCOMPARE(updates.childAt(0)["file"].data(), QString("C:/work/src/main.cpp"));
}

void DebuggerUnitTests::testCdbImplBreakpointStopMessages()
{
    const auto at = [](int line, const QString &message, bool tracepoint) {
        BreakpointParameters params(BreakpointByFileAndLine);
        params.fileName = FilePath::fromUserInput("C:/src/main.cpp");
        params.textPosition.line = line;
        params.message = message;
        params.tracepoint = tracepoint;
        return params;
    };

    // A plain breakpoint with a message: the message is the user's to see, and
    // the session stays stopped.
    QHash<QString, BreakpointParameters> inserted{{"3", at(42, "reached the loop", false)}};
    BreakpointStopMessages messages = breakpointStopMessages(inserted, "3");
    QCOMPARE(messages.plainMessage, QString("reached the loop"));
    QVERIFY(messages.tracepointMessages.isEmpty());

    // One without a message logs nothing.
    inserted = {{"3", at(42, {}, false)}};
    messages = breakpointStopMessages(inserted, "3");
    QVERIFY(messages.plainMessage.isEmpty());
    QVERIFY(messages.tracepointMessages.isEmpty());

    // A tracepoint's message goes through the tracepoint path instead, which
    // expands it against the inferior, and the session resumes.
    inserted = {{"3", at(42, "i is {i}", true)}};
    messages = breakpointStopMessages(inserted, "3");
    QCOMPARE(messages.tracepointMessages, QStringList{"i is {i}"});
    QVERIFY(messages.plainMessage.isEmpty());
    QVERIFY(!messages.stopAfterwards);

    // A tracepoint and a breakpoint on the same line are one cdb breakpoint, so
    // the tracepoint is expanded and the session stops afterwards anyway.
    inserted = {{"3", at(42, "i is {i}", true)}, {"4", at(42, "reached the loop", false)}};
    messages = breakpointStopMessages(inserted, "3");
    QCOMPARE(messages.tracepointMessages, QStringList{"i is {i}"});
    QVERIFY(messages.stopAfterwards);
    // The plain one's message is not logged twice: the tracepoint path reports.
    QVERIFY(messages.plainMessage.isEmpty());

    // A breakpoint somewhere else has nothing to do with this stop.
    inserted = {{"3", at(42, "reached the loop", false)}, {"4", at(99, "i is {i}", true)}};
    messages = breakpointStopMessages(inserted, "3");
    QCOMPARE(messages.plainMessage, QString("reached the loop"));
    QVERIFY(messages.tracepointMessages.isEmpty());
}

void DebuggerUnitTests::testCdbImplStepIntoLanding()
{
    const auto stopData = [](const QString &frame) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        GdbMi data;
        data.fromString("{stack=[{" + frame + "}]}", decoder);
        return data;
    };
    const auto always = [](const QString &) { return true; };
    const auto never = [](const QString &) { return false; };

    // The step arrived in a function whose sources are here. Show it.
    QCOMPARE(stepIntoLanding(stopData(R"(fullname="C:/src/main.cpp",function="run")"), always),
             StepIntoLanding::Arrived);

    // cdb stops on the import thunk before the call reaches the function, so
    // one more step is needed to get there.
    QCOMPARE(stepIntoLanding(stopData(R"(fullname="",function="tst!_imp_ILT+35_runfoo")"),
                             always),
             StepIntoLanding::OnThunk);

    // A function with no source at all, so stepping into it showed disassembly.
    QCOMPARE(stepIntoLanding(stopData(R"(fullname="",function="ntdll!RtlAllocateHeap")"), always),
             StepIntoLanding::WithoutSource);

    // A pdb naming a source file that was never shipped to this machine is the
    // same thing: there is nothing to show.
    QCOMPARE(stepIntoLanding(stopData(R"(fullname="C:/qt/src/qstring.cpp",function="op")"),
                             never),
             StepIntoLanding::WithoutSource);

    // Without a stack there is nothing to decide on, and stepping further would
    // run the program away from wherever it is.
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi empty;
    empty.fromString(R"({reason="exception"})", decoder);
    QCOMPARE(stepIntoLanding(empty, never), StepIntoLanding::Arrived);
}

void DebuggerUnitTests::testCdbImplScriptMessages()
{
    const auto reply = [](const QString &contents) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        GdbMi data;
        data.fromString('{' + contents + '}', decoder);
        return data;
    };

    // What the dumper printed on its way to an answer, alongside the answer.
    QCOMPARE(scriptMessages(reply(R"(result="...",msg=["no such type","2 items shown"])")),
             QStringList({"no such type", "2 items shown"}));

    // The bridge wraps its own in a named tuple.
    QCOMPARE(scriptMessages(reply(R"(msg=[bridgemessage={msg="dumper is loaded"}])")),
             QStringList{"dumper is loaded"});

    // A quiet reply says nothing, and neither do we.
    QVERIFY(scriptMessages(reply(R"(result="...")")).isEmpty());

    // An empty entry is not worth a log line of its own.
    QVERIFY(scriptMessages(reply(R"(msg=[""])")).isEmpty());
}

void DebuggerUnitTests::testCdbImplBreakpointInsertCommand()
{
    const auto atLine = [] {
        BreakpointParameters params(BreakpointByFileAndLine);
        params.fileName = FilePath::fromUserInput("C:/src/main.cpp");
        params.textPosition.line = 42;
        return params;
    };
    // cdb takes a file name the way the device the file lives on spells it, so
    // the expected commands carry that spelling rather than a fixed one.
    const QString source = FilePath::fromUserInput("C:/src/main.cpp").nativePath();
    const QString buildSource = FilePath::fromUserInput("X:/buildsrv/main.cpp").nativePath();

    // The plain cases, as cdb spells them.
    BreakpointParameters params = atLine();
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("bu7 `" + source + ":42`"));

    params.module = "app.exe";
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("bu7 `app.exe!" + source + ":42`"));

    // Break on a memory address, offered as "Break on Memory Address".
    params = BreakpointParameters(BreakpointByAddress);
    params.address = 0x401000;
    QCOMPARE(breakpointInsertCommand(params, "7", {}), QString("bu7 0x401000"));

    // A watchpoint reads and writes, and a size of its own.
    params = BreakpointParameters(WatchpointAtAddress);
    params.address = 0x401000;
    params.size = 4;
    QCOMPARE(breakpointInsertCommand(params, "7", {}), QString("ba7 r4 0x401000"));

    // Break when a new process is executed: cdb has no such event, so it is a
    // breakpoint on what starts one.
    params = BreakpointParameters(BreakpointAtExec);
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("bu7 kernel32!CreateProcessW"));

    // Break when "main" starts, and only the first time round.
    params = BreakpointParameters(BreakpointAtMain);
    params.module = "app.exe";
    QCOMPARE(breakpointInsertCommand(params, "7", {}), QString("bu7 /1 app.exe!main"));

    // The thread a breakpoint is restricted to comes before the command.
    params = atLine();
    params.threadSpec = 2;
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("~2 bu7 `" + source + ":42`"));

    // cdb counts the passes it takes to stop, the view counts the ones to let by.
    params = atLine();
    params.ignoreCount = 3;
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("bu7 `" + source + ":42` 0n4"));

    // What to run on a hit.
    params = atLine();
    params.command = ".echo here";
    QCOMPARE(breakpointInsertCommand(params, "7", {}),
             QString("bu7 `" + source + ":42` \".echo here\""));

    // The file name cdb matches is the one the inferior was built from, so the
    // source path mapping has to be applied the other way round.
    params = atLine();
    const QList<QPair<QString, QString>> map{{"X:/buildsrv", "C:/src"}};
    QCOMPARE(breakpointInsertCommand(params, "7", map),
             QString("bu7 `" + buildSource + ":42`"));

    // Unless the user asked for the file name alone.
    params.pathUsage = BreakpointUseShortPath;
    QCOMPARE(breakpointInsertCommand(params, "7", map), QString("bu7 `main.cpp:42`"));

    // A function resolved to an address keeps everything but its location.
    params = BreakpointParameters(BreakpointByFunction);
    params.functionName = "runFoo";
    params.ignoreCount = 1;
    QCOMPARE(breakpointInsertCommand(params, "7", {}, "0x401000"),
             QString("bu7 0x401000 0n2"));

    // Nothing cdb could break on.
    QVERIFY(breakpointInsertCommand(BreakpointParameters(BreakpointAtFork), "7", {}).isEmpty());
}

void DebuggerUnitTests::testCdbImplBreakpointModuleScope()
{
    // What gets loaded, spelled the way cdb names a module.
    QCOMPARE(cdbModuleName({FilePath::fromUserInput("C:/build/my-app.exe")}),
             QString("my_app"));
    QCOMPARE(cdbModuleName({FilePath::fromUserInput("C:/build/core.dll")}), QString("core"));

    // An import library is not a module, so the one DLL beside it still counts.
    QCOMPARE(cdbModuleName({FilePath::fromUserInput("C:/build/core.lib"),
                            FilePath::fromUserInput("C:/build/core.dll")}),
             QString("core"));

    // A file built into more than one binary names no single module, and one
    // built into none names nothing either.
    QVERIFY(cdbModuleName({FilePath::fromUserInput("C:/build/a.dll"),
                           FilePath::fromUserInput("C:/build/b.dll")}).isEmpty());
    QVERIFY(cdbModuleName({}).isEmpty());
    QVERIFY(cdbModuleName({FilePath::fromUserInput("C:/build/core.lib")}).isEmpty());

    const auto always = [](const FilePath &) { return QString("core"); };
    BreakpointParameters params(BreakpointByFileAndLine);
    params.fileName = FilePath::fromUserInput("C:/src/main.cpp");
    params.textPosition.line = 42;

    // Without a module cdb resolves the location right away and refuses a
    // breakpoint in a library that has not been loaded yet.
    QCOMPARE(scopedToModule(params, always).module, QString("core"));
    QCOMPARE(breakpointInsertCommand(scopedToModule(params, always), "7", {}),
             QString("bu7 `core!" + params.fileName.nativePath() + ":42`"));

    // What the user asked for wins over what the project says.
    params.module = "other";
    QCOMPARE(scopedToModule(params, always).module, QString("other"));

    // Only a breakpoint by file and line is placed this way.
    BreakpointParameters byFunction(BreakpointByFunction);
    byFunction.functionName = "runFoo";
    QVERIFY(scopedToModule(byFunction, always).module.isEmpty());

    // A session that cannot say, an attach without a project, leaves it alone.
    params.module.clear();
    QVERIFY(scopedToModule(params, {}).module.isEmpty());
}

void DebuggerUnitTests::testCdbImplDisassemblyRange()
{
    // The addresses of what a name matched, taken from cdb's own spelling.
    const QString reply = "00007ffb`842b1000 core!runFoo (void)\n"
                          "00007ffb`842b2000 core!runFoo (int)\n";
    QCOMPARE(symbolAddresses(reply), QList<quint64>({0x7ffb842b1000, 0x7ffb842b2000}));
    QVERIFY(symbolAddresses("Couldn't resolve error at 'core!runFoo'").isEmpty());

    // A name with no address to go by starts the disassembly at the function.
    QCOMPARE(disassemblyRange(0, {0x1000}).start, quint64(0x1000));
    QCOMPARE(disassemblyRange(0, {0x1000}).end, quint64(0x1100));
    QVERIFY(disassemblyRange(0, {}).isEmpty());

    // An address within a function starts at the function: an x86 instruction
    // has no length to find a boundary by, so a window that begins in the
    // middle of one decodes to something that is not the program.
    QCOMPARE(disassemblyRange(0x1040, {0x1000}).start, quint64(0x1000));
    QCOMPARE(disassemblyRange(0x1040, {0x1000}).end, quint64(0x1140));

    // The closest of the overloads below the address is the one it is in.
    QCOMPARE(disassemblyRange(0x2040, {0x1000, 0x2000, 0x3000}).start, quint64(0x2000));

    // Past what fits, the window wins over the start of a long function.
    QCOMPARE(disassemblyRange(0x2000, {0x1000}).start, quint64(0x1f00));

    // A function that begins above the address is not the one it is in, and
    // neither is anything at all when the name could not be resolved.
    QCOMPARE(disassemblyRange(0x1000, {0x2000}).start, quint64(0xf00));
    QCOMPARE(disassemblyRange(0x1000, {}).start, quint64(0xf00));
    QCOMPARE(disassemblyRange(0x1000, {}).end, quint64(0x1100));

    // An address the window reaches below zero from.
    QCOMPARE(disassemblyRange(0x40, {}).start, quint64(0));
}

void DebuggerUnitTests::testCdbImplNormalizedSourceFileName()
{
    const auto missing = [](const QString &) { return false; };
    const auto present = [](const QString &) { return true; };

    // A file that is there is spelled the way it is spelled on disk, which is
    // what normalizedPathName() answers with.
    QCOMPARE(normalizedSourceFileName("c:/src/main.cpp", nullptr, present),
             QString("c:/src/main.cpp"));

    // Without a file to ask, at least the drive letter is the one the editor
    // uses, so that a frame in a source that is not around does not open a
    // second editor for a file another frame named upper case.
    QCOMPARE(normalizedSourceFileName("c:/src/main.cpp", nullptr, missing),
             QString("C:/src/main.cpp"));
    QCOMPARE(normalizedSourceFileName("c:/src/./sub/../main.cpp", nullptr, missing),
             QString("C:/src/main.cpp"));
    QCOMPARE(normalizedSourceFileName("", nullptr, missing), QString());
    QCOMPARE(normalizedSourceFileName("/src/main.cpp", nullptr, missing),
             QString("/src/main.cpp"));

    // Normalizing goes to the file system, so an answer is given once.
    QHash<QString, QString> cache;
    QCOMPARE(normalizedSourceFileName("c:/src/main.cpp", &cache, missing),
             QString("C:/src/main.cpp"));
    QCOMPARE(cache.size(), 1);
    QCOMPARE(normalizedSourceFileName("c:/src/main.cpp", &cache, present),
             QString("C:/src/main.cpp"));
    QCOMPARE(cache.size(), 1);
}

void DebuggerUnitTests::testCdbImplModulesTree()
{
    const auto reply = [](const QString &contents) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        GdbMi data;
        data.fromString('[' + contents + ']', decoder);
        return data;
    };

    // The extension names a module by its cdb name and its addresses in hex, the
    // modules view reads gdb's names and decimal.
    const GdbMi modules = modulesTree(
        reply(R"({name="tst_inferior",image="C:\\build\\tst_inferior.exe",)"
              R"(start="0x7ff61f3a0000",end="0x7ff61f3affff"},)"
              R"({name="KERNEL32",image="C:\\Windows\\system32\\KERNEL32.DLL",)"
              R"(start="0x7ffb842b0000",end="0x7ffb843edfff",deferred="true"})"));
    QCOMPARE(modules.childCount(), 2);
    QCOMPARE(modules.childAt(0)["modulepath"].data(),
             QString("C:\\build\\tst_inferior.exe"));
    QCOMPARE(modules.childAt(0)["startaddress"].data().toULongLong(),
             quint64(0x7ff61f3a0000));
    QCOMPARE(modules.childAt(0)["endaddress"].data().toULongLong(),
             quint64(0x7ff61f3affff));
    QCOMPARE(modules.childAt(0)["symbolsread"].data(), QString("Yes"));

    // A module whose symbols cdb has not read yet says so.
    QCOMPARE(modules.childAt(1)["symbolsread"].data(), QString("No"));

    // A reply that is not a list of modules is no modules.
    QCOMPARE(modulesTree(reply({})).childCount(), 0);
}

void DebuggerUnitTests::testCdbImplRegistersTree()
{
    // cdb names a register's type by its width, and Register::guessMissingData()
    // reads the kind off the names gdb uses.
    QCOMPARE(registerTypeName("I64"), QString("int"));
    QCOMPARE(registerTypeName("I8"), QString("int"));
    QCOMPARE(registerTypeName("F80"), QString("float"));
    QCOMPARE(registerTypeName("V128"), QString("vec"));
    QCOMPARE(registerTypeName(""), QString());

    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi reply;
    reply.fromString(R"([{number="0",name="rax",size="8",type="I64",value="0x0"},)"
                     R"({number="1",name="xmm0",size="16",type="V128",value="0x0"}])",
                     decoder);
    const GdbMi registers = registersTree(reply);
    QCOMPARE(registers.childCount(), 2);
    QCOMPARE(registers.childAt(0)["type"].data(), QString("int"));
    QCOMPARE(registers.childAt(1)["type"].data(), QString("vec"));

    // Everything else the view needs is passed on as it was reported.
    QCOMPARE(registers.childAt(0)["name"].data(), QString("rax"));
    QCOMPARE(registers.childAt(0)["size"].data(), QString("8"));
    QCOMPARE(registers.childAt(1)["value"].data(), QString("0x0"));

    // What the kind is read from, with what the register view then makes of it.
    Register reg;
    reg.name = "rax";
    reg.reportedType = registers.childAt(0)["type"].data();
    reg.guessMissingData();
    QCOMPARE(reg.kind, IntegerRegister);
    reg.reportedType = registers.childAt(1)["type"].data();
    reg.kind = UnknownRegister;
    reg.guessMissingData();
    QCOMPARE(reg.kind, VectorRegister);
}

void DebuggerUnitTests::testCdbImplSetParameterArguments()
{
    QCOMPARE(setParameterArguments(true, false),
             QString("setparameter firstChance=1 secondChance=0 maxStackDepth=25"));
    QCOMPARE(setParameterArguments(false, true),
             QString("setparameter firstChance=0 secondChance=1 maxStackDepth=25"));

    // The stack a stop reports is only read as deep as the scan for a native to
    // QML boundary goes, and that one needs 25 frames.
    const QString depth = setParameterArguments(true, true).section("maxStackDepth=", 1, 1);
    bool ok = false;
    QCOMPARE(depth.toInt(&ok), 25);
    QVERIFY(ok);
}

// A session without a build configuration - an attach, or a foreign debug
// adapter - has none, and the debugger's own path stands in for one.
void DebuggerUnitTests::testMapsAnEmptyFileNameToNothing()
{
    Kit *kit = KitManager::defaultKit();
    QVERIFY(kit);

    const std::unique_ptr<RunControl> runControl(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    runControl->setKit(kit);
    const DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl.get());
    if (rp.buildDirectory().isEmpty())
        QSKIP("The kit has no debugger to stand in for a build directory.");

    // What a stack frame that names no file would otherwise be opened from.
    QVERIFY(rp.mapToProjectPath({}).isEmpty());
}

void DebuggerUnitTests::testCdbSourcePathMapping()
{
    const QList<QPair<QString, QString>> native{{"X:\\buildsrv", "C:\\src"}};
    QCOMPARE(cdbSourcePathMapping("C:/src/foo.cpp", native, SourceToDebugger),
             QString("X:\\buildsrv/foo.cpp"));
    QCOMPARE(cdbSourcePathMapping("X:/buildsrv/foo.cpp", native, DebuggerToSource),
             QString("C:\\src/foo.cpp"));

    const QList<QPair<QString, QString>> portable{{"X:/buildsrv", "C:/src"}};
    QCOMPARE(cdbSourcePathMapping("C:\\src\\foo.cpp", portable, SourceToDebugger),
             QString("X:/buildsrv\\foo.cpp"));

    QCOMPARE(cdbSourcePathMapping("c:/SRC/foo.cpp", native, SourceToDebugger),
             QString("X:\\buildsrv/foo.cpp"));

    QCOMPARE(cdbSourcePathMapping("C:/srcery/foo.cpp", native, SourceToDebugger),
             QString("C:/srcery/foo.cpp"));

    QCOMPARE(cdbSourcePathMapping("C:/src/foo.cpp", {}, SourceToDebugger),
             QString("C:/src/foo.cpp"));
}

void DebuggerUnitTests::testCdbBreakpointFileName()
{
    BreakpointParameters params(BreakpointByFileAndLine);
    params.fileName = FilePath::fromUserInput("C:/src/foo.cpp");
    params.textPosition.line = 42;

    const QList<QPair<QString, QString>> mapping{{"X:/buildsrv", "C:/src"}};
    const QString mapped = HostOsInfo::isWindowsHost() ? QString("X:\\buildsrv\\foo.cpp")
                                                       : QString("X:/buildsrv/foo.cpp");
    QCOMPARE(cdbAddBreakpointCommand(params, mapping, "100000"),
             QString("bu100000 `%1:42`").arg(mapped));

    // What reaches cdb is spelled the way the file's own device spells it,
    // which is not necessarily the way the host does.
    const QString unmapped = HostOsInfo::isWindowsHost() ? QString("C:\\src\\foo.cpp")
                                                         : QString("C:/src/foo.cpp");
    QCOMPARE(cdbAddBreakpointCommand(params, {}, "100000"),
             QString("bu100000 `%1:42`").arg(unmapped));

    // A short path takes part in no mapping at all.
    params.pathUsage = BreakpointUseShortPath;
    QCOMPARE(cdbAddBreakpointCommand(params, mapping, "100000"),
             QString("bu100000 `foo.cpp:42`"));
}

static QByteArray debugStrings(const QStringList &strings)
{
    QByteArray result;
    for (const QString &string : strings)
        result += string.toUtf8() + '\0';
    return result;
}

void DebuggerUnitTests::testQtBuildSourceRoots_data()
{
    QTest::addColumn<QByteArray>("blob");
    QTest::addColumn<QStringList>("roots");

    QTest::newRow("empty") << QByteArray() << QStringList();

    QTest::newRow("no marker")
        << debugStrings({"/usr/include/stdio.h", "int"}) << QStringList();

    QTest::newRow("one root")
        << debugStrings({"/home/qt/work/qt/qtbase/src/corelib/global/qglobal.h"})
        << QStringList{"/home/qt/work/qt"};

    QTest::newRow("directory only")
        << debugStrings({"/opt/src/qt/qtbase/src/corelib/global"})
        << QStringList{"/opt/src/qt"};

    QTest::newRow("two roots")
        << debugStrings({"/build/a/qtbase/src/corelib/kernel/qobject.cpp",
                         "/build/b/qtbase/src/gui/kernel/qwindow.cpp"})
        << QStringList{"/build/a", "/build/b"};

    QTest::newRow("same root twice")
        << debugStrings({"/build/a/qtbase/src/corelib/kernel/qobject.cpp",
                         "/build/a/qtbase/src/gui/kernel/qwindow.cpp"})
        << QStringList{"/build/a"};

    QTest::newRow("relative path")
        << debugStrings({"../../qtbase/src/corelib/global/qglobal.h"}) << QStringList();

    QTest::newRow("empty root")
        << debugStrings({"/qtbase/src/corelib/global/qglobal.h"}) << QStringList();

    QTest::newRow("marker in a neighbor string")
        << debugStrings({"/build/a", "/qtbase/src/corelib"}) << QStringList();
}

void DebuggerUnitTests::testQtBuildSourceRoots()
{
    QFETCH(QByteArray, blob);
    QFETCH(QStringList, roots);

    QCOMPARE(qtBuildSourceRoots(blob), roots);
}

void DebuggerUnitTests::testDebugInfoDirectory()
{
    DebuggerRunParameters rp;

    // Without a location of its own, gdb's default is used, whether or not the
    // parameters were enriched.
    QCOMPARE(debugInfoDirectory(rp), FilePath::fromString("/usr/lib/debug"));

    rp.setSysRoot(FilePath::fromString("/sysroots/target"));
    QCOMPARE(debugInfoDirectory(rp), FilePath::fromString("/sysroots/target/usr/lib/debug"));

    rp.setDebugInfoLocation(FilePath::fromString("/elsewhere/debug"));
    QCOMPARE(debugInfoDirectory(rp), FilePath::fromString("/elsewhere/debug"));
}

void DebuggerUnitTests::testDebugInfoFile()
{
    if (HostOsInfo::isWindowsHost()) {
        QSKIP("The lookup is for ELF binaries, and the debug file directory candidate "
              "cannot be built from a path carrying a drive letter.");
    }

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath root = FilePath::fromString(tmp.path());
    const FilePath libDir = root / "lib";
    const FilePath library = libDir / "libQt6Core.so.6";
    const FilePath debugInfoDir = root / "debug";
    const QByteArray link = "libQt6Core.so.6.debug";
    const QByteArray buildId = "abcdef0123";

    QVERIFY(libDir.ensureWritableDir());
    QVERIFY(library.ensureExistingFile());

    // Nothing to find yet, and neither a missing link nor a stub of a build id
    // may send it anywhere else.
    QCOMPARE(debugInfoFile(library, link, buildId, debugInfoDir), library);
    QCOMPARE(debugInfoFile(library, {}, {}, debugInfoDir), library);
    QCOMPARE(debugInfoFile(library, link, "ab", debugInfoDir), library);

    // Create the candidates in reverse order of precedence, so each one has to
    // take over from the one created before it.
    const FilePath byBuildId = debugInfoDir / ".build-id" / "ab" / "cdef0123.debug";
    const FilePath global = debugInfoDir.pathAppended(libDir.path()) / QString::fromUtf8(link);
    const FilePath subdir = libDir / ".debug" / QString::fromUtf8(link);
    const FilePath adjacent = libDir / QString::fromUtf8(link);

    for (const FilePath &candidate : {global, subdir, adjacent, byBuildId}) {
        QVERIFY(candidate.parentDir().ensureWritableDir());
        QVERIFY(candidate.ensureExistingFile());
        QCOMPARE(debugInfoFile(library, link, buildId, debugInfoDir), candidate);
    }

    // Without a debug information directory only the candidates next to the
    // library remain, and the build id cannot be resolved at all.
    QCOMPARE(debugInfoFile(library, link, buildId, {}), adjacent);
    QCOMPARE(debugInfoFile(library, {}, buildId, {}), library);
}

void DebuggerUnitTests::testMergePlatformQtPath()
{
    const QString sources = "/home/dev/qt-src";
    const QString elsewhere = "/elsewhere";
    const SourcePathMap empty;

    // A build root read from the debug information is mapped onto the sources.
    QCOMPARE(mergePlatformQtPath(sources, {"/build/qt"}, empty).value("/build/qt"), sources);

    // A developer build records the source location itself. Mapping that onto
    // itself is pointless, so it is skipped.
    QVERIFY(!mergePlatformQtPath(sources, {sources}, empty).contains(sources));

    // A user setting for the same root wins.
    SourcePathMap user;
    user.insert("/build/qt", elsewhere);
    QCOMPARE(mergePlatformQtPath(sources, {"/build/qt"}, user).value("/build/qt"), elsewhere);

    // The hardcoded build paths are added even without a root from the debug
    // information, and are left alone where the user has set one.
    const SourcePathMap platform = mergePlatformQtPath(sources, {}, empty);
    QVERIFY(!platform.isEmpty());
    for (auto it = platform.cbegin(), end = platform.cend(); it != end; ++it)
        QCOMPARE(it.value(), sources);

    SourcePathMap platformUser;
    platformUser.insert(platform.firstKey(), elsewhere);
    QCOMPARE(mergePlatformQtPath(sources, {}, platformUser).value(platform.firstKey()),
             elsewhere);
}

void DebuggerUnitTests::testNormalizedSourcePathPrefix()
{
    // The prefix is matched against the debug information, so nothing but the
    // separators may be touched. In particular a leading "./" has to survive.
    QCOMPARE(normalizedSourcePathPrefix("  ./ogr  "), QString("./ogr"));
    QCOMPARE(normalizedSourcePathPrefix("/build/../qt"), QString("/build/../qt"));

    // Backslashes go, on every host: a Windows prefix can be entered while
    // debugging a Windows target from a Linux host.
    QCOMPARE(normalizedSourcePathPrefix("C:\\work\\qt"), QString("C:/work/qt"));
    QCOMPARE(normalizedSourcePathPrefix("(C:\\work\\.*)\\src"), QString("(C:/work/.*)/src"));
}

// A backend that records what it is asked to do and answers only when told to,
// so the engine's side of a breakpoint's life can be driven a step at a time.
class RecordingBackend : public DebuggerEngineInterface
{
public:
    RecordingBackend() : DebuggerEngineInterface(setupData()) {}

    static DebuggerEngineSetupData setupData()
    {
        DebuggerEngineSetupData data;
        data.startModes = DebuggerStartModeFlag::Launch;
        data.acceptsBreakpoint = [](const AcceptsBreakpointQuery &) { return true; };
        return data;
    }

    void changeBreakpoint(const BreakpointChangeRequest &request) override
    {
        requests.append(request);
    }

    void answerLastAsInterpreter(BreakpointOp op, const QString &number)
    {
        QVERIFY(!requests.isEmpty());
        const BreakpointChangeRequest request = requests.last();
        // What the QML debug service reports: its own spelling, and the model
        // id the request went out with.
        GdbMi bkpt;
        bkpt.m_type = GdbMi::Tuple;
        const auto add = [&bkpt](const QString &name, const QString &data) {
            GdbMi child;
            child.m_type = GdbMi::Const;
            child.m_name = name;
            child.m_data = data;
            bkpt.addChild(child);
        };
        add("number", number);
        add("enabled", "1");
        add("condition", QString());
        add("ignorecount", "0");
        add("line", "10");
        add("modelid", QString::number(request.modelId));
        add("pending", "0");
        GdbMi list;
        list.m_type = GdbMi::List;
        list.addChild(bkpt);
        emit breakpointEvent(request.requestId, op, true, list);
    }

    QList<BreakpointChangeRequest> requests;

    void start() override {}
    void shutdownInferior(ShutdownMode) override {}
    void shutdownEngine() override {}
    void execute(const ExecutionRequest &) override {}
    void refresh(const RefreshRequest &) override {}
    void accessMemory(MemoryOp, quint64, quint64, quint64, const QByteArray &) override {}
    void executeDebuggerCommand(const QString &, const WatchItemData &) override {}
    void selectThread(const QString &) override {}
    void activateFrame(int) override {}
    void fetchDisassembly(quint64, quint64, const QString &) override {}
    void assignValueInDebugger(const WatchItemData &, const QString &, const QString &) override {}
    void setRegisterValue(const QString &, const QString &) override {}
    void setPeripheralRegisterValue(quint64, quint64) override {}
    void watchPoint(quint64, const QPoint &) override {}
    void createSnapshot(quint64) override {}
};

// Claims one breakpoint for a fresh engine and answers the insert the way the
// QML debug service does. Returns the engine's own item for it.
static Breakpoint claimedInterpreterBreakpoint(GenericDebuggerEngine *engine,
                                               RecordingBackend *backend)
{
    BreakpointParameters params;
    params.type = BreakpointByFileAndLine;
    params.fileName = FilePath::fromUserInput("Main.qml");
    params.textPosition = {10, -1};
    params.enabled = true;
    BreakpointManager::createBreakpoint(params);
    BreakpointManager::claimBreakpointsForEngine(engine);

    if (backend->requests.isEmpty())
        return {};
    backend->answerLastAsInterpreter(BreakpointOp::Insert, "1");
    return engine->breakHandler()->findBreakpointByModelId(backend->requests.last().modelId);
}

// The service spells an enabled breakpoint "1", gdb spells it "y". Read as gdb
// output, the reply to the insert turns the breakpoint off the moment it is
// made, and nothing the user does in the view can turn it back on.
void DebuggerUnitTests::testInterpreterBreakpointStaysEnabled()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    const Breakpoint bp = claimedInterpreterBreakpoint(engine, backend);
    QVERIFY(bp);
    QVERIFY2(bp->isEnabled(), "the interpreter's reply turned the breakpoint off");
    QCOMPARE(bp->responseId(), QString("1"));
}

// Insertion and removal both say they are proceeding, and the state machine
// takes the answer only from there. An update that skips it leaves the
// breakpoint stuck, so the view never shows what the user asked for.
void DebuggerUnitTests::testBreakpointUpdateAnnouncesItIsProceeding()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    const Breakpoint bp = claimedInterpreterBreakpoint(engine, backend);
    QVERIFY(bp);
    QCOMPARE(bp->state(), BreakpointInserted);

    engine->breakHandler()->requestBreakpointUpdate(bp);
    QCOMPARE(bp->state(), BreakpointUpdateProceeding);
}

// A program small enough to say exactly where a step should land in it.
static const char s_steppingSource[] = R"CPP(
int addOne(int value)
{
    int raised = value + 1; // MARKER: in-callee
    return raised;
}

int main()
{
    int first = 1;
    int second = addOne(first); // MARKER: at-call
    int third = second + first; // MARKER: after-call
    return third - third;
}
)CPP";

// Builds that program and runs it up to a marked line, holding it there so a
// test can ask for a step and see where it ends up. It needs no project: a
// compiler and a kit to name the debugger are the whole of it.
class SteppingSession
{
public:
    SteppingSession()
    {
        // Nothing here can answer a dialog, and the engine puts one up for a
        // breakpoint it cannot place. A breakpoint that does not take shows
        // up as a session that never stops where it was told to.
        m_warnedAboutBreakpoints = settings().showUnsupportedBreakpointWarning();
        settings().showUnsupportedBreakpointWarning.setValue(false);
    }

    ~SteppingSession()
    {
        settings().showUnsupportedBreakpointWarning.setValue(m_warnedAboutBreakpoints);
        if (m_runControl) {
            m_runControl->initiateStop();
            QTestEventLoop::instance().enterLoop(30);
        }
        if (m_breakpoint)
            m_breakpoint->deleteBreakpoint();
    }

    // Empty when the session is up, otherwise why it is not.
    QString start(const QString &marker)
    {
        if (!m_dir.isValid())
            return "no temporary directory to build in";

        const auto buildsForThisMachine = [](Kit *candidate) {
            if (!candidate->isValid())
                return false;
            Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(candidate);
            return toolchain && toolchain->targetAbi() == Abi::hostAbi();
        };
        Kit *kit = Utils::findOr(KitManager::kits(), nullptr, buildsForThisMachine);
        if (!kit)
            return "no kit of this machine's own architecture to build with";
        Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(kit);
        if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
            return "building the inferior is only wired up for gcc-style compilers";

        const FilePath dir = FilePath::fromString(m_dir.path());
        m_source = dir / "stepping.cpp";
        m_executable = (dir / "stepping").withExecutableSuffix();
        if (!m_source.writeFileContents(QByteArray(s_steppingSource)))
            return "could not write " + m_source.toUserOutput();

        m_line = markerLine(marker);
        if (m_line <= 0)
            return "no line marked " + marker;

        Process compiler;
        compiler.setCommand({toolchain->compilerCommand(),
                             {"-g", "-O0", m_source.nativePath(),
                              "-o", m_executable.nativePath()}});
        compiler.runBlocking(std::chrono::seconds(60));
        if (compiler.exitCode() != 0 || !m_executable.isExecutableFile())
            return "the inferior would not build: " + compiler.allOutput().left(300);

        BreakpointParameters params;
        params.type = BreakpointByFileAndLine;
        params.fileName = m_source;
        params.textPosition = {m_line, -1};
        params.enabled = true;
        m_breakpoint = BreakpointManager::createBreakpoint(params);

        const QList<QPointer<DebuggerEngine>> before = EngineManager::engines();
        m_runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        m_runControl->setKit(kit);
        DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(m_runControl);
        rp.setInferior(ProcessRunData{CommandLine{m_executable}, dir});
        QObject::connect(m_runControl, &RunControl::stopped,
                         &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
        m_runControl->setRunRecipe(debuggerRecipe(m_runControl, rp));
        m_runControl->start();

        // EngineManager holds every engine the application ever made, and other
        // tests leave theirs behind, so take the one that was not there before.
        const bool arrived = QTest::qWaitFor([this, &before] {
            for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
                if (candidate && !before.contains(candidate))
                    m_engine = candidate;
            }
            // Stopped is not yet arrived: the stack the test reads about comes
            // in after the state does.
            return m_engine && m_engine->state() == InferiorStopOk
                   && m_engine->stackHandler()->currentFrame().line == m_line;
        }, 90000);
        if (!arrived)
            return "the session never stopped at " + marker;
        return {};
    }

    DebuggerEngine *engine() const { return m_engine; }
    int line() const { return m_line; }

    int markerLine(const QString &marker) const
    {
        const QStringList lines = QString::fromUtf8(s_steppingSource).split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            if (lines.at(i).contains(marker))
                return i + 1;
        }
        return 0;
    }

private:
    QTemporaryDir m_dir;
    FilePath m_source;
    FilePath m_executable;
    RunControl *m_runControl = nullptr;
    QPointer<DebuggerEngine> m_engine;
    GlobalBreakpoint m_breakpoint;
    bool m_warnedAboutBreakpoints = false;
    int m_line = 0;
};

// Asks for a step and waits for the engine to settle somewhere else. Reports
// where it ended up, which is what the test has to say something about.
static StackFrame stepAndSettle(DebuggerEngine *engine, void (DebuggerEngine::*ask)(),
                                QString *complaint)
{
    const StackFrame before = engine->stackHandler()->currentFrame();
    (engine->*ask)();
    const bool moved = QTest::qWaitFor([engine, before] {
        const StackFrame now = engine->stackHandler()->currentFrame();
        return engine->state() == InferiorStopOk
               && (now.function != before.function || now.line != before.line);
    }, 30000);
    const StackFrame after = engine->stackHandler()->currentFrame();
    if (!moved) {
        *complaint = QString("the step never arrived - state %1, still at %2:%3")
                         .arg(engine->state()).arg(after.function).arg(after.line);
    }
    return after;
}

// The generic backends are what these exercise. Nothing here can say anything
// about them when the environment has turned them off.
static QString reasonTheGenericBackendsAreNotUnderTest()
{
    if (isUseGenericDebuggerOverride() && !useGenericDebuggerEnabled())
        return "QTC_USE_GENERIC_DEBUGGER turns the backends under test off.";
    return {};
}

void DebuggerUnitTests::testStepsIntoACalledFunction()
{
    if (const QString reason = reasonTheGenericBackendsAreNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    const bool wasOn = commonSettings().useGenericDebugger();
    commonSettings().useGenericDebugger.setValue(true);
    const QScopeGuard restore([wasOn] { commonSettings().useGenericDebugger.setValue(wasOn); });

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    QString complaint;
    const StackFrame frame = stepAndSettle(session.engine(),
                                           &DebuggerEngine::handleExecStepIn, &complaint);
    QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
    // Backends spell a function with its signature or without it.
    QVERIFY2(frame.function.startsWith("addOne"),
             qPrintable(QString("stepping in at the call landed in %1:%2")
                            .arg(frame.function).arg(frame.line)));
    QCOMPARE(frame.line, session.markerLine("MARKER: in-callee"));
}

void DebuggerUnitTests::testStepsOverACallWithoutEnteringIt()
{
    if (const QString reason = reasonTheGenericBackendsAreNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    const bool wasOn = commonSettings().useGenericDebugger();
    commonSettings().useGenericDebugger.setValue(true);
    const QScopeGuard restore([wasOn] { commonSettings().useGenericDebugger.setValue(wasOn); });

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    QString complaint;
    const StackFrame frame = stepAndSettle(session.engine(),
                                           &DebuggerEngine::handleExecStepOver, &complaint);
    QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
    QVERIFY2(frame.function.startsWith("main"),
             qPrintable(QString("stepping over the call left main for %1:%2")
                            .arg(frame.function).arg(frame.line)));
    QCOMPARE(frame.line, session.markerLine("MARKER: after-call"));
}

void DebuggerUnitTests::testStepsOutOfACalledFunction()
{
    if (const QString reason = reasonTheGenericBackendsAreNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    const bool wasOn = commonSettings().useGenericDebugger();
    commonSettings().useGenericDebugger.setValue(true);
    const QScopeGuard restore([wasOn] { commonSettings().useGenericDebugger.setValue(wasOn); });

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    QVERIFY(session.engine()->stackHandler()->currentFrame().function.startsWith("addOne"));

    QString complaint;
    const StackFrame frame = stepAndSettle(session.engine(),
                                           &DebuggerEngine::handleExecStepOut, &complaint);
    QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
    QVERIFY2(frame.function.startsWith("main"),
             qPrintable(QString("stepping out of addOne landed in %1:%2")
                            .arg(frame.function).arg(frame.line)));
}

void DebuggerUnitTests::testScratchEditorAdoptsSavedName()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    // Declared after the directory so the editor is gone before the file it
    // was saved to.
    const QScopeGuard cleanup([] { EditorManager::closeAllEditors(false); });

    openTextEditor("Backtrace$", "line1\nline2\n");
    IEditor *editor = EditorManager::currentEditor();
    QVERIFY(editor);
    IDocument *doc = editor->document();
    QVERIFY(doc);
    QVERIFY(!doc->preferredDisplayName().isEmpty());
    QVERIFY(doc->isTemporary());

    QVERIFY(DocumentManager::saveDocument(doc, FilePath::fromString(tmp.path()) / "bt.txt"));
    QVERIFY(doc->preferredDisplayName().isEmpty());
    QVERIFY(!doc->isTemporary());
}

void DebuggerUnitTests::testNamespaceFromQObjectRtti_data()
{
    QTest::addColumn<QByteArray>("symbol");
    QTest::addColumn<QString>("expected");

    QTest::newRow("typeinfo") << QByteArray("_ZTIN4MyNs7QObjectE") << QString("MyNs");
    QTest::newRow("typename") << QByteArray("_ZTSN4MyNs7QObjectE") << QString("MyNs");
    QTest::newRow("vtable") << QByteArray("_ZTVN4MyNs7QObjectE") << QString("MyNs");
    QTest::newRow("nested") << QByteArray("_ZTIN1A1B7QObjectE") << QString("A::B");
    QTest::newRow("two-digit-length") << QByteArray("_ZTIN10MyLongerNs7QObjectE")
                                      << QString("MyLongerNs");
    QTest::newRow("other-class") << QByteArray("_ZTIN4MyNs14QObjectPrivateE") << QString();
    QTest::newRow("no-components") << QByteArray("_ZTIN0E") << QString();
    QTest::newRow("empty-nesting") << QByteArray("_ZTINE") << QString();
    QTest::newRow("not-nested") << QByteArray("_ZTI7QObject") << QString();
    QTest::newRow("length-past-end") << QByteArray("_ZTIN18MyNS7QObjectE") << QString();
    QTest::newRow("length-misaligned") << QByteArray("_ZTIN9Short7QObjectE") << QString();
    QTest::newRow("length-overflow") << QByteArray("_ZTIN99999999999999999997QObjectE")
                                     << QString();
    QTest::newRow("length-wrapping") << QByteArray("_ZTIN18446744073709551617A7QObjectE")
                                     << QString();
}

void DebuggerUnitTests::testNamespaceFromQObjectRtti()
{
    QFETCH(QByteArray, symbol);
    QFETCH(QString, expected);

    QCOMPARE(namespaceFromQObjectRtti(symbol), expected);
}

void DebuggerUnitTests::testTerminateMessage()
{
    // All three ways the C++ runtime announces a std::terminate, not just the
    // uncaught exception that causes most of them.
    QVERIFY(isTerminateMessage(u"terminate called after throwing an instance of 'int'"));
    QVERIFY(isTerminateMessage(u"terminate called without an active exception"));
    QVERIFY(isTerminateMessage(u"terminate called recursively"));

    // The message arrives mixed into the inferior's own output, and on the
    // collector path still wrapped in gdb's stream record.
    QVERIFY(isTerminateMessage(u"computing...\nterminate called recursively\n"));
    QVERIFY(isTerminateMessage(u"&\"terminate called recursively\\n\""));

    QVERIFY(!isTerminateMessage(u"Application exited with exit code 1"));
    QVERIFY(!isTerminateMessage(u""));
}

QObject *createDebuggerTest()
{
    return new DebuggerUnitTests;
}

} // Debugger::Internal

#include "debuggertest.moc"

#endif // WITH_TESTS
