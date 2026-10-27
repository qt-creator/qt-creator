// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "debuggertest.h"

#include "breakhandler.h"
#include "cdb/cdbengine.h"
#include "cdb/cdbimpl.h"
#include "cdb/cdbparsehelpers.h"
#include "debuggeractions.h"
#include "disassembleragent.h"
#include "disassemblerlines.h"
#include "debuggercore.h"
#include "debuggerengine.h"
#include "debuggerengineinterface.h"
#include "debuggerinternalconstants.h"
#include "debuggeritem.h"
#include "debuggerkitaspect.h"
#include "debuggerrunconfigurationaspect.h"
#include "debuggerruncontrol.h"
#include "debuggersourcepathmappingwidget.h"
#include "enginemanager.h"
#include "logwindow.h"
#include "mcpsessionstate.h"
#include "mcpsupport.h"
#include "mcpsupport_p.h"
#include "qml/qmlengine.h"
#include "qml/qmlinspectoragent.h"
#include "gdb/gdbengine.h"
#include "genericdebuggerengine.h"
#include "registerhandler.h"
#include "shared/hostutils.h"
#include "stackframe.h"
#include "commonoptionspage.h"
#include "stackhandler.h"
#include "watchhandler.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>

#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/abi.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>

#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitaspect.h>

#include <texteditor/texteditor.h>

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/qtcprocess.h>
#include <utils/store.h>
#include <utils/url.h>

#include <QElapsedTimer>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMouseEvent>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QTcpServer>
#include <QTemporaryDir>
#include <QTest>
#include <QTestEventLoop>
#include <QTextBlock>
#include <QTimer>
#include <QVersionNumber>
#include <QtEndian>

#include <memory>
#include <optional>

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
    void testWideRegisterValues();

    void testDebuggerLanguages_data();
    void testDebuggerLanguages();

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
    void testFindsASourceFileOnTheDebuggerDevice();
    void testCdbSourcePathMapping();
    void testCdbSourcePathMappingToHost();
    void testCdbSourceFileNameOnDevice();
    void testCdbBreakpointFileName();
    void testCdbEnvironmentScript();
    void testCdbEnvironmentScriptOnDevice();

    void testQtBuildSourceRoots_data();
    void testQtBuildSourceRoots();
    void testPdbSourceFileNames();
    void testPdbSourceFileNamesOnDevice();
    void testPeDebugStrings();

    void testDebugInfoDirectory();
    void testDebugInfoFile();
    void testMergePlatformQtPath();
    void testNormalizedSourcePathPrefix();

    void testStepsIntoACalledFunction();
    void testStepsOverACallWithoutEnteringIt();
    void testStepsOutOfACalledFunction();
    void testMcpStepWaitsForTheNewStop_data();
    void testMcpStepWaitsForTheNewStop();
    void testMcpOperationIsNotCompleteBeforeTheStop_data();
    void testMcpOperationIsNotCompleteBeforeTheStop();
    void testMcpResumeThatFailsKeepsTheStop_data();
    void testMcpResumeThatFailsKeepsTheStop();
    void testMcpSelectFrameWaitsForItsLocals_data();
    void testMcpSelectFrameWaitsForItsLocals();
    void testMcpStackFollowsTheThreadThatStopped_data();
    void testMcpStackFollowsTheThreadThatStopped();
    void testMcpLocalsOutlastFramesAddedToTheStack_data();
    void testMcpLocalsOutlastFramesAddedToTheStack();
    void testMcpSelectFrameBeforeTheStopsLocals_data();
    void testMcpSelectFrameBeforeTheStopsLocals();
    void testLocalsOfAFrameSelectedEarlyKeepNothingOfTheOtherFrame_data();
    void testLocalsOfAFrameSelectedEarlyKeepNothingOfTheOtherFrame();
    void testMcpVariableFetchesItsChildren_data();
    void testMcpVariableFetchesItsChildren();
    void testMcpCollectsTheStacksOfAllThreads_data();
    void testMcpCollectsTheStacksOfAllThreads();
    void testMcpThreadStacksAreBounded_data();
    void testMcpThreadStacksAreBounded();
    void testMcpStackOfAStopInAnotherThread_data();
    void testMcpStackOfAStopInAnotherThread();
    void testMcpSelectThreadWaitsForItsStack_data();
    void testMcpSelectThreadWaitsForItsStack();
    void testMcpReadsTheRegistersOfTheCurrentStop_data();
    void testMcpReadsTheRegistersOfTheCurrentStop();
    void testMcpRegistersOfAnOuterFrame_data();
    void testMcpRegistersOfAnOuterFrame();
    void testMcpReadsAVectorRegister_data();
    void testMcpReadsAVectorRegister();
    void testMemoryRetryParts();
    void testGdbRegisterValueAsHex_data();
    void testGdbRegisterValueAsHex();
    void testMcpMemoryAccessPolicy_data();
    void testMcpMemoryAccessPolicy();
    void testMcpReadsMemory_data();
    void testMcpReadsMemory();
    void testMcpReadsMemoryUpToAnUnmappedPage_data();
    void testMcpReadsMemoryUpToAnUnmappedPage();
    void testMcpEvaluatesAnExpressionOfTheCurrentStop_data();
    void testMcpEvaluatesAnExpressionOfTheCurrentStop();
    void testMcpEvaluationKeepsTheUsersWatch_data();
    void testMcpEvaluationKeepsTheUsersWatch();
    void testMcpEvaluationsShareATemporaryWatch_data();
    void testMcpEvaluationsShareATemporaryWatch();
    void testMcpReportsCapabilities_data();
    void testMcpReportsCapabilities();
    void testDisabledBreakpointDoesNotStop_data();
    void testDisabledBreakpointDoesNotStop();
    void testDisassemblyThatMissesTheAddressMarksNoLine();
    void testOnlyMachineCodeIsOfferedADisassembly();
    void testCompiledQmlFramesAreNative();
    void testGdbEngineTakesCompiledQmlBreakpoints();
    void testABreakpointDropsThePlacesItNoLongerStandsIn();
    void testClaimedQmlBreakpointsGetNoQmlHint();
    void testUnhandledQmlBreakpointsGetAHint();
    void testKitWithCompiledQmlDisablesQmlDebugging();
    void testAnEmptyDisassemblyLeavesTheViewAlone();
    void testDisassemblyLinksToItsSource();
    void testMissingSourceMessage();
    void testASkippedTopFrameNamesItsMissingSource();
    void testSkippedMachineryFramesStaySilent();
    void testAnEmptiedStackHasNoCurrentFrame();
    void testAFailedRunRestoresOnlyAPendingReset();
    void testAResumedAttachStopLeavesTheViewsAlone();
    void testARefusedResumeReadsTheAttachStopAfterAll();
    void testASuccessfulRunDropsTheUnreadAttachStop();
    void testTheDisassemblyMessageIsSaidOncePerStop();

    void testScratchEditorAdoptsSavedName();
    void testBreakpointUpdateAnnouncesItIsProceeding();
    void testInterpreterBreakpointStaysEnabled();
    void testBreakpointFileNameSpelledAsOnDisk();
    void testUnresolvedLegacyLanguagesSurviveASave();
    void testLegacyQmlAndPythonKeepsBothLanguages();
    void testJavaOnlyEnablesNoOtherDebugger();
    void testAutomaticDebugsAJavaLaunchAsJava();
    void testNativeMixedEnvironmentVariableWins();
    void testOverrideKitDebugger();
    void testPluginBringsTheBackend();
    void testPluginFailsToBringTheBackend();
    void testPluginBackendNeedsCppDebugging();
    void testCombinedEngineNeedsNoQmlChannel();
    void testQmlInspectorLeavesParentlessObjectsCollapsed();
    void testInspectorFollowsParentlessObjects();
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

static Kit *kitWithAQt()
{
    return Utils::findOr(KitManager::kits(), nullptr, [](Kit *kit) {
        return kit->isValid() && QtSupport::QtKitAspect::qtVersion(kit);
    });
}

void DebuggerUnitTests::testStateMachine()
{
    FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");

    Kit *kit = kitWithAQt();
    if (!kit)
        QSKIP("This test needs a kit with a Qt to build its project with.");
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile, kit));

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

    Kit *kit = kitWithAQt();
    if (!kit)
        QSKIP("This test needs a kit with a Qt to build its project with.");
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile, kit));

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
    // The DAP mode under test is gdb's, and the kit's debugger is whatever the
    // machine has: on a macOS kit that is lldb, which answers none of it.
    if (DebuggerKitAspect::engineType(kit) != GdbEngineType)
        QSKIP("The debugger of this kit is not gdb.");
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
    // 512-bit (AVX-512 ZMM): used to be cut to its lower 256 bits.
    QTest::newRow("512bit")
        << "0f1e2d3c4b5a69788796a5b4c3d2e1f0" "112233445566778899aabbccddeeff00"
           "99aabbccddeeff001122334455667788" "fedcba98765432100123456789abcdef";
    QTest::newRow("512bit-high-only") << "deadbeef" + QString(120, '0');
    QTest::newRow("512bit-all-f") << QString(128, 'f');
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

    // What a debugger says instead of a value is not a value of zero.
    RegisterValue unavailable;
    unavailable.fromString("<unavailable>", HexadecimalFormat);
    QVERIFY(!unavailable.known);
    RegisterValue zero;
    zero.fromString("0x0", HexadecimalFormat);
    QVERIFY(zero.known);
    QVERIFY(!(unavailable == zero));
}

void DebuggerUnitTests::testWideRegisterValues()
{
    // A negative value carries through all four lanes.
    RegisterValue minusOne;
    minusOne.fromString("0x-1", HexadecimalFormat);
    QCOMPARE(minusOne.toString(IntegerRegister, 64, HexadecimalFormat), QString(128, 'f'));

    // Decimal input carries into the upper lanes too.
    RegisterValue twoPow256;
    twoPow256.fromString("115792089237316195423570985008687907853269984665640564039457584007913129639936",
                         DecimalFormat);
    QCOMPARE(twoPow256.toString(IntegerRegister, 64, HexadecimalFormat),
             QString(63, '0') + '1' + QString(64, '0'));
    QCOMPARE(twoPow256.subValue(16, 2).v.u128[0], Quint128(1));
    QCOMPARE(twoPow256.subValue(16, 3).v.u128[0], Quint128(0));

    // A 512-bit register is reported with all its bits, a wider one not at all.
    Register zmm;
    zmm.name = "zmm0";
    zmm.size = 64;
    zmm.value.fromString("0xdeadbeef" + QString(120, '0'), HexadecimalFormat);
    const QJsonObject wide = mcpRegisterToJson(zmm, true);
    QVERIFY(wide.value("available").toBool());
    QCOMPARE(wide.value("bit_width").toInt(), 512);
    QCOMPARE(wide.value("value").toString(), "0xdeadbeef" + QString(120, '0'));
    Register wider = zmm;
    wider.size = 128;
    const QJsonObject tooWide = mcpRegisterToJson(wider, true);
    QVERIFY(!tooWide.value("available").toBool());
    QVERIFY(!tooWide.contains("value"));
}

// What a run configuration stores is the name of the selection, and what it
// means is read back off the index of it, so the two lists have to line up.
void DebuggerUnitTests::testDebuggerLanguages_data()
{
    QTest::addColumn<QString>("stored");
    QTest::addColumn<bool>("cpp");
    QTest::addColumn<bool>("qml");
    QTest::addColumn<bool>("python");
    QTest::addColumn<bool>("java");

    QTest::newRow("Cpp") << "Cpp" << true << false << false << false;
    QTest::newRow("Qml") << "Qml" << false << true << false << false;
    QTest::newRow("CppAndQml") << "CppAndQml" << true << true << false << false;
    QTest::newRow("CppAndQmlCombined") << "CppAndQmlCombined" << true << true << false << false;
    QTest::newRow("Python") << "Python" << false << false << true << false;
    QTest::newRow("CppAndPython") << "CppAndPython" << true << false << true << false;
    QTest::newRow("QmlAndPython") << "QmlAndPython" << false << true << true << false;
    QTest::newRow("CppQmlAndPython") << "CppQmlAndPython" << true << true << true << false;
    QTest::newRow("Java") << "Java" << false << false << false << true;
    QTest::newRow("CppAndJava") << "CppAndJava" << true << false << false << true;
    QTest::newRow("QmlAndJava") << "QmlAndJava" << false << true << false << true;
    QTest::newRow("CppQmlAndJava") << "CppQmlAndJava" << true << true << false << true;
}

void DebuggerUnitTests::testDebuggerLanguages()
{
    QFETCH(QString, stored);
    QFETCH(bool, cpp);
    QFETCH(bool, qml);
    QFETCH(bool, python);
    QFETCH(bool, java);

    // The build configuration is only read where the selection is automatic,
    // which is none of the rows above.
    DebuggerRunConfigurationAspect aspect(nullptr);
    Store map;
    map.insert("RunConfiguration.DebuggerLanguages", stored);
    aspect.fromMap(map);

    QCOMPARE(aspect.isAutomatic(), false);
    QCOMPARE(aspect.useCppDebugger(), cpp);
    QCOMPARE(aspect.useQmlDebugger(), qml);
    QCOMPARE(aspect.usePythonDebugger(), python);
    QCOMPARE(aspect.useJavaDebugger(), java);
    // The one selection that puts both languages on a single engine.
    QCOMPARE(aspect.useCombinedEngine(), stored == "CppAndQmlCombined");
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

    // A cdb on a device is started by a batch file that sets the environment, and the
    // extension is loaded by its absolute path, from a startup script. Without a TEMP
    // on the device, both are staged next to the inferior.
    {
        CdbImplStartData deviceData = startData;
        const FilePath cdbExecutable = FilePath::fromParts(u"ssh", u"somehost", u"C:/dbg/cdb.exe");
        deviceData.debuggerRunData.command = CommandLine(cdbExecutable);

        CdbImpl cdb(deviceData);
        QVERIFY(cdb.setupProcess());
        const CommandLine cmd = cdb.m_cdbProc.commandLine();
        QCOMPARE(cmd.executable(), cdbExecutable.withNewPath("cmd.exe"));
        QCOMPARE(cmd.arguments(),
                 "/d /v:off /c \"" + cdb.m_environmentScriptFile.nativePath() + '"');
        QCOMPARE(cdb.m_environmentScriptFile.parentDir(), inferiorDir);
        const QByteArray environmentScript
            = cdb.m_environmentScriptFile.fileContents().value_or(QByteArray());
        QVERIFY(environmentScript.contains("set QT_LOGGING_TO_CONSOLE=0\r\n"));
        const QString cdLine = "cd /d \"" + inferiorDir.nativePath() + "\"\r\n";
        QVERIFY(environmentScript.contains(cdLine.toUtf8()));

        const QList<QByteArray> scriptLines = environmentScript.split('\n');
        const auto cdbLine = std::find_if(scriptLines.cbegin(), scriptLines.cend(),
                                          [](const QByteArray &line) {
                                              return line.startsWith("set QTC_CDB=");
                                          });
        QVERIFY(cdbLine != scriptLines.cend());
        QString launch = QString::fromUtf8(cdbLine->mid(12).chopped(1)).replace("%%", "%");
        launch.replace(QRegularExpression("\\^(.)"), "\\1");
        const QString exePart = '"' + cdbExecutable.nativePath() + "\" ";
        QVERIFY(launch.startsWith(exePart));
        const QStringList args = CommandLine(cdbExecutable, launch.mid(exePart.size()),
                                             CommandLine::Raw).splitArguments();
        QVERIFY(!args.contains("-aqtcreatorcdbext.dll"));
        QVERIFY(!args.contains("-c"));
        const int scriptIndex = args.indexOf("-cf") + 1;
        QVERIFY(scriptIndex > 0);
        QCOMPARE(args.at(scriptIndex), cdb.m_initScriptFile.nativePath());
        QCOMPARE(cdb.m_initScriptFile.parentDir(), inferiorDir);
        const QByteArray script = cdb.m_initScriptFile.fileContents().value_or(QByteArray());
        QCOMPARE(QString::fromLocal8Bit(script),
                 ".load " + extension.nativePath() + '\n' + idleCommand + '\n');

        // A reset restart reuses the command line, so the scripts stay until the
        // session really ends. A local stand-in for cdb ends right away, and the
        // restarted one ends before it ever came up.
        const FilePath environmentScriptFile = cdb.m_environmentScriptFile;
        const FilePath initScriptFile = cdb.m_initScriptFile;
        cdb.m_cdbProc.setCommand(HostOsInfo::isWindowsHost()
                                     ? CommandLine{"cmd.exe", {"/c", "exit"}}
                                     : CommandLine{"true"});
        cdb.m_isResetRestart = true;
        cdb.m_initialSessionIdleHandled = true;
        int doneCount = 0;
        bool scriptsKept = false;
        QObject::connect(&cdb.m_cdbProc, &Process::done, &cdb, [&] {
            if (++doneCount == 1)
                scriptsKept = environmentScriptFile.exists() && initScriptFile.exists();
        });
        cdb.m_cdbProc.start();
        QTRY_COMPARE(doneCount, 2);
        QVERIFY(scriptsKept);
        QVERIFY(!environmentScriptFile.exists());
        QVERIFY(!initScriptFile.exists());

        // An engine destroyed while cdb runs never sees it end, and removes the scripts
        // itself.
        FilePath stagedByImpl[2];
        {
            CdbImpl tornDown(deviceData);
            QVERIFY(tornDown.setupProcess());
            stagedByImpl[0] = tornDown.m_environmentScriptFile;
            stagedByImpl[1] = tornDown.m_initScriptFile;
            QVERIFY(stagedByImpl[0].exists() && stagedByImpl[1].exists());
        }
        QVERIFY(!stagedByImpl[0].exists());
        QVERIFY(!stagedByImpl[1].exists());
        const FilePath stagedByEngine = inferiorDir / "qtc-cdb-engine.cmd";
        QVERIFY(stagedByEngine.writeFileContents("@echo off\r\n"));
        {
            CdbEngine engine;
            engine.m_environmentScriptFile = stagedByEngine;
        }
        QVERIFY(!stagedByEngine.exists());
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

    // A source in several modules names each of them.
    QCOMPARE(cdbModuleNames({FilePath::fromUserInput("C:/build/a.dll"),
                             FilePath::fromUserInput("C:/build/b.lib"),
                             FilePath::fromUserInput("C:/build/b.dll")}),
             QStringList({"a", "b"}));

    // Binaries cdb cannot tell apart are one module, not one each.
    QCOMPARE(cdbModuleNames({FilePath::fromUserInput("C:/build/my-app.exe"),
                             FilePath::fromUserInput("C:/build/my+app.dll")}),
             QStringList("my_app"));

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

void DebuggerUnitTests::testFindsASourceFileOnTheDebuggerDevice()
{
    Kit *kit = KitManager::defaultKit();
    QVERIFY(kit);

    const std::unique_ptr<RunControl> runControl(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    runControl->setKit(kit);
    const DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl.get());

    const bool wasOn = commonSettings().lookUpSourcesOnDebuggerDevice();
    commonSettings().lookUpSourcesOnDebuggerDevice.setValue(true);
    const QScopeGuard restore(
        [wasOn] { commonSettings().lookUpSourcesOnDebuggerDevice.setValue(wasOn); });

    // The debugger binary itself is the one file the debugger's device is
    // known to have, so it stands in for a source only that device can see.
    const FilePath debugger = rp.debugger().command.executable();
    const FilePath dir = debugger.parentDir();
    if (!debugger.isReadableFile() || dir.fileName().isEmpty())
        QSKIP("The kit has no readable debugger below a named directory.");

    QCOMPARE(rp.findOnDebuggerDevice(debugger.path()), debugger);

    const QString viaParent = dir.path() + "/../" + dir.fileName() + "/" + debugger.fileName();
    QCOMPARE(rp.findOnDebuggerDevice(viaParent), debugger);

    // A relative name is rejected outright, and the only way to tell that
    // from failing the readability probe is to name a file that does exist
    // where a relative name would be resolved.
    const FilePaths here = FilePath::currentWorkingPath().dirEntries(DirFilterFlag::Files);
    if (here.isEmpty())
        QSKIP("The current directory has no file to name relatively.");
    QVERIFY(rp.findOnDebuggerDevice(here.first().fileName()).isEmpty());

    // Turning the lookup off has to reach both routes, so it sits in the one
    // place they share rather than in either engine's cleanupFullName().
    commonSettings().lookUpSourcesOnDebuggerDevice.setValue(false);
    QVERIFY(rp.findOnDebuggerDevice(debugger.path()).isEmpty());
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

// A remote cdb names the files of its device. A mapped name the host has stays
// on the host, one it misses is looked up on the device and stays local when
// the device lacks it, too. The mapped name keeps cdb's separators after the
// mapped part, which only a Windows host would accept as they are.
void DebuggerUnitTests::testCdbSourcePathMappingToHost()
{
    QTemporaryDir hostDir;
    QVERIFY(hostDir.isValid());
    const FilePath hostFile = FilePath::fromString(hostDir.path()) / "main.cpp";
    QVERIFY(hostFile.writeFileContents("int main() {}\n"));

    CdbEngine engine;
    DebuggerRunParameters rp;
    ProcessRunData cdb;
    cdb.command = CommandLine(FilePath::fromString("ssh://windows/C:/Debuggers/x64/cdb.exe"));
    rp.setDebugger(cdb);
    engine.setRunParameters(rp);
    engine.m_sourcePathMappings.push_back({"C:/build/src", hostDir.path()});

    const bool wasOn = commonSettings().lookUpSourcesOnDebuggerDevice();
    const QScopeGuard restore(
        [wasOn] { commonSettings().lookUpSourcesOnDebuggerDevice.setValue(wasOn); });
    commonSettings().lookUpSourcesOnDebuggerDevice.setValue(true);

    const auto mapped = engine.sourceMapNormalizeFileNameFromDebugger("C:\\build\\src\\main.cpp");
    QVERIFY(mapped.fileName.isLocal());
    QVERIFY(mapped.fileName.isSameFile(hostFile));
    QVERIFY(mapped.exists);

    const auto missing = engine.sourceMapNormalizeFileNameFromDebugger("C:\\build\\src\\gone.cpp");
    QVERIFY(missing.fileName.isLocal());
    QCOMPARE(missing.fileName.fileName(), QString("gone.cpp"));
    QVERIFY(!missing.exists);

    commonSettings().lookUpSourcesOnDebuggerDevice.setValue(false);

    const auto unmapped = engine.sourceMapNormalizeFileNameFromDebugger("c:\\other\\x\\..\\a.cpp");
    QVERIFY(unmapped.fileName.isLocal());
    QCOMPARE(unmapped.fileName.path(), QString("C:/other/a.cpp"));
    QVERIFY(!unmapped.exists);
}

static const char noWindowsTestDevice[]
    = "Set QTC_SSH_TEST_WIN_HOST (and _USER/_PORT/_KEYFILE) to a Windows-over-SSH host, "
      "and load the Remote plugin.";

// Registers a device for the Windows-over-SSH test host, if there is one.
static IDevicePtr addWindowsTestDevice()
{
    const SshParameters params = SshTest::getParameters("WIN");
    if (!SshTest::hasVariantHost("WIN") || !SshTest::checkParameters(params))
        return {};
    IDeviceFactory *factory
        = Utils::findOrDefault(IDeviceFactory::allDeviceFactories(), [](IDeviceFactory *f) {
              return f->deviceType() == Id("GenericWindowsOsType");
          });
    if (!factory)
        return {};

    const IDevicePtr device = factory->construct();
    QTC_ASSERT(device, return {});
    device->sshParametersAspectContainer().setSshParameters(params);
    DeviceManager::addDevice(device);
    return device;
}

static bool connectToDevice(const IDevicePtr &device)
{
    QEventLoop loop;
    QTimer::singleShot(60 * 1000, &loop, [&loop] { loop.exit(1); });
    device->tryToConnect(Continuation<>(&loop, [&loop](const Result<> &res) {
        loop.exit(res ? 0 : 1);
    }));
    return loop.exec() == 0;
}

// A file that only the device of a remote cdb has is found there.
void DebuggerUnitTests::testCdbSourceFileNameOnDevice()
{
    const IDevicePtr device = addWindowsTestDevice();
    if (!device)
        QSKIP(noWindowsTestDevice);
    const QScopeGuard cleanup([id = device->id()] { DeviceManager::removeDevice(id); });
    QVERIFY2(connectToDevice(device), "Cannot connect to the device.");

    const bool wasOn = commonSettings().lookUpSourcesOnDebuggerDevice();
    commonSettings().lookUpSourcesOnDebuggerDevice.setValue(true);
    const QScopeGuard restore(
        [wasOn] { commonSettings().lookUpSourcesOnDebuggerDevice.setValue(wasOn); });

    CdbEngine engine;
    DebuggerRunParameters rp;
    ProcessRunData cdb;
    cdb.command = CommandLine(device->rootPath().withNewPath("C:/Debuggers/x64/cdb.exe"));
    rp.setDebugger(cdb);
    engine.setRunParameters(rp);

    const auto onDevice
        = engine.sourceMapNormalizeFileNameFromDebugger("c:\\windows\\system32\\..\\win.ini");
    QCOMPARE(onDevice.fileName, device->rootPath().withNewPath("C:/windows/win.ini"));
    QVERIFY(onDevice.exists);

    // A mapping may point to the device, too.
    const Result<FilePath> tmp = device->rootPath().tmpDir();
    if (!tmp)
        QFAIL(qPrintable(tmp.error()));
    const Result<FilePath> dir = tmp->pathAppended("debuggertest-XXXXXX").createTempDir();
    if (!dir)
        QFAIL(qPrintable(dir.error()));
    const QScopeGuard removeDir([dir] { dir->removeRecursively(); });
    QVERIFY(dir->pathAppended("main.cpp").writeFileContents("int main() {}\n"));
    engine.m_sourcePathMappings.push_back({"C:/build", dir->path()});
    const auto mapped = engine.sourceMapNormalizeFileNameFromDebugger("C:\\build\\main.cpp");
    QCOMPARE(mapped.fileName, dir->pathAppended("main.cpp"));
    QVERIFY(mapped.exists);
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

void DebuggerUnitTests::testCdbEnvironmentScript()
{
    const Environment device({"SystemRoot=C:\\Windows", "KEPT=1", "UNSET=2", "DISABLED=3",
                              "LONGKEPT=" + QString(9000, 'x')},
                             OsTypeWindows);
    Environment env = device;
    env.unset("UNSET");
    env.set("DISABLED", "3", false);
    env.set("QUOTED", "\"C:\\Program Files\";a&b|c<d>e(f)^");
    env.set("PERCENT", "50%");
    env.set("NEWLINE", "a\nb");
    env.set("LONG", QString(5000, '('));

    QStringList skipped;
    const CommandLine cdb(FilePath::fromParts(u"ssh", u"somehost", u"C:/dbg/cdb.exe"),
                          "-cf \"C:\\R&D 50%\\init.cmd\"", CommandLine::Raw);
    const Result<QByteArray> scriptResult = cdbEnvironmentScript(env, device, {}, cdb, &skipped);
    QVERIFY(scriptResult);
    const QByteArray script = *scriptResult;
    // A variable the device has already is inherited, even one too long to be set.
    QVERIFY(!script.contains("KEPT"));
    QVERIFY(script.contains("set UNSET=\r\n"));
    QVERIFY(script.contains("set DISABLED=\r\n"));
    QVERIFY(script.contains("set PERCENT=50%%\r\n"));
    QVERIFY(script.contains("set QUOTED=^\"C:\\Program Files^\";a^&b^|c^<d^>e^(f^)^^\r\n"));
    QVERIFY(!script.contains("NEWLINE"));
    QVERIFY(!script.contains("LONG"));
    QCOMPARE(skipped, QStringList({"LONG", "NEWLINE"}));
    // cdb runs last, after the code page is restored, from a line read before that.
    QVERIFY(script.contains(QByteArray("set QTC_CDB=^\"" + cdb.executable().nativePath().toUtf8()
                                       + "^\" -cf ^\"C:\\R^&D 50%%\\init.cmd^\"\r\n")));
    QVERIFY(script.endsWith("\"%QTC_CHCP%\" %QTC_OLD_CP% > nul\r\n"
                            "set QTC_CHCP=\r\nset QTC_OLD_CP=\r\n"
                            "set \"QTC_CDB=\" & %QTC_CDB%\r\n"));

    // A cdb command line that cmd.exe cannot take is an error, not a skipped variable.
    const CommandLine longCdb(cdb.executable(), QString(5000, '('), CommandLine::Raw);
    QVERIFY(!cdbEnvironmentScript(env, device, {}, longCdb));
    const CommandLine brokenCdb(cdb.executable(), "a\nb", CommandLine::Raw);
    QVERIFY(!cdbEnvironmentScript(env, device, {}, brokenCdb));

    // The default shell of the device parses the command line before cmd.exe does.
    const FilePath scriptDir = FilePath::fromParts(u"ssh", u"somehost", u"C:/Users/A B/Temp");
    const FilePath cmdExe = scriptDir.withNewPath("C:/Windows/System32/cmd.exe");
    const Result<CommandLine> command = cdbDeviceCommandLine(scriptDir / "x.cmd", cmdExe);
    QVERIFY(command);
    QCOMPARE(command->executable(), cmdExe);
    QCOMPARE(command->arguments(),
             "/d /v:off /c \"" + (scriptDir / "x.cmd").nativePath() + '"');
    QVERIFY(!cdbDeviceCommandLine(scriptDir.withNewPath("C:/R&D/x.cmd"), cmdExe));
    // PowerShell expands a '$' in a double-quoted string.
    QVERIFY(!cdbDeviceCommandLine(scriptDir.withNewPath("C:/Users/svc$x/x.cmd"), cmdExe));

    const Environment withTemp({"TEMP=C:\\Users\\A B\\AppData\\Local\\Temp"}, OsTypeWindows);
    QCOMPARE(cdbScriptDir(cdb.executable(), withTemp),
             cdb.executable().withNewPath("C:/Users/A B/AppData/Local/Temp"));
    QVERIFY(cdbScriptDir(cdb.executable(), Environment(OsTypeWindows)).isEmpty());
    // The script then goes next to the inferior.
    const Environment withSpecialTemp({"TEMP=C:\\Users\\R&D\\AppData\\Local\\Temp"},
                                      OsTypeWindows);
    QVERIFY(cdbScriptDir(cdb.executable(), withSpecialTemp).isEmpty());

    // cmd.exe ignores the case of a key, a host-typed env does not.
    const Environment windowsDevice({"SystemRoot=C:\\Windows", "Path=C:\\Windows"},
                                    OsTypeWindows);
    Environment linuxEnv(OsTypeLinux);
    linuxEnv.set("SYSTEMROOT", "C:\\Windows");
    for (const QString &path : {QString("C:\\Qt\\bin"), QString("C:\\Windows")}) {
        linuxEnv.set("PATH", path);
        const Result<QByteArray> linuxScript
            = cdbEnvironmentScript(linuxEnv, windowsDevice, {}, {});
        QVERIFY(linuxScript);
        const QStringList lines = QString::fromUtf8(*linuxScript).split("\r\n");
        const QStringList pathLines = Utils::filtered(lines, [](const QString &line) {
            return line.startsWith("set PATH=", Qt::CaseInsensitive);
        });
        // With the value of the device, PATH is inherited from Path.
        QCOMPARE(pathLines, path == windowsDevice.value("Path") ? QStringList()
                                                                : QStringList("set PATH=" + path));
    }

    // A staged script that cmd.exe cannot be pointed at is removed again.
    QTemporaryDir stageDir;
    QVERIFY(stageDir.isValid());
    const FilePath specialDir = FilePath::fromString(stageDir.path()) / "R&D";
    QVERIFY(specialDir.ensureWritableDir());
    const FilePath staged = specialDir / "x.cmd";
    const Result<CdbDeviceStart> refused = cdbDeviceStart(
        cdb, env, device, {}, [&staged](const QByteArray &contents, const FilePath &) {
            return staged.writeFileContents(contents).transform([&staged](qint64) {
                return staged;
            });
        });
    QVERIFY(!refused);
    QVERIFY(specialDir.exists());
    QVERIFY(!staged.exists());

    // A run environment that was not built on the one of the device only adds to it.
    Environment additions(OsTypeWindows);
    additions.set("QT_LOGGING_TO_CONSOLE", "0");
    const Result<QByteArray> additionsScript = cdbEnvironmentScript(additions, device, {}, {});
    QVERIFY(additionsScript);
    QVERIFY(additionsScript->contains("set QT_LOGGING_TO_CONSOLE=0\r\n"));
    QVERIFY(!additionsScript->contains("set SystemRoot="));
    QVERIFY(!additionsScript->contains("set KEPT="));
    // Prepending to PATH keeps the entries of the device, whatever the case of the key.
    Environment pathAdditions(OsTypeWindows);
    pathAdditions.prependOrSet("PATH", "C:\\Qt\\bin");
    const Result<QByteArray> pathScript
        = cdbEnvironmentScript(pathAdditions, windowsDevice, {}, {});
    QVERIFY(pathScript);
    const QStringList pathScriptLines = Utils::filtered(
        QString::fromUtf8(*pathScript).split("\r\n"),
        [](const QString &line) { return line.startsWith("set PATH=", Qt::CaseInsensitive); });
    QCOMPARE(pathScriptLines, QStringList("set Path=C:\\Qt\\bin;C:\\Windows"));
}

// The batch file holds up in a real cmd.exe, run the way a cdb on a device is.
void DebuggerUnitTests::testCdbEnvironmentScriptOnDevice()
{
    const IDevicePtr device = addWindowsTestDevice();
    if (!device)
        QSKIP(noWindowsTestDevice);
    const QScopeGuard cleanup([id = device->id()] { DeviceManager::removeDevice(id); });
    QVERIFY2(connectToDevice(device), "Cannot connect to the device.");

    const FilePath root = device->rootPath();
    const Environment deviceEnvironment = root.deviceEnvironment();
    QVERIFY(deviceEnvironment.hasKey("USERNAME"));
    QVERIFY(deviceEnvironment.hasKey("COMPUTERNAME"));
    const FilePath tempDir
        = root.withNewPath(FilePath::fromUserInput(deviceEnvironment.value("TEMP")).path());
    QVERIFY(tempDir.isDir());

    Environment env = deviceEnvironment;
    env.set("QTC_TEST_PERCENT", "100% sure %NOPE");
    env.set("QTC_TEST_BANG", "a!PATH!b");
    env.set("QTC_TEST_META", "\"C:\\Program Files\";\"a&b|c<d>e(f)^");
    env.set("QTC_TEST_UMLAUT", QString::fromUtf8("Gr\xc3\xbc\xc3\x9f" "e"));
    const QString quotedPathEntry = "\"C:\\Qtc Test;Dir\"";
    // Without System32, which the restoring chcp in the batch file must not depend on.
    env.set("PATH", quotedPathEntry);
    env.unset("USERNAME");
    env.set("COMPUTERNAME", deviceEnvironment.value("COMPUTERNAME"), false);
    const FilePath workDir = tempDir / "qtc a&b";
    QVERIFY(workDir.ensureWritableDir());
    const QScopeGuard removeWorkDir([workDir] { workDir.removeRecursively(); });

    // The stand-in for cdb gets an argument that is not safe in cmd.exe and not ASCII.
    // The commands after it run in the batch file, once the code page is restored.
    const FilePath cmdExe = root.findCmdExe(deviceEnvironment);
    const QString launchArgs = QString::fromUtf8("/d /c echo \"QTC_ARG=a&b \xc3\xbc\"")
                               + " & set & cd & "
                               + cmdExe.parentDir().pathAppended("chcp.com").nativePath();
    const CommandLine launch(cmdExe, launchArgs, CommandLine::Raw);
    // An ssh session may well be in 65001 already. Start from another code page, which
    // the batch file has to switch away from and back to.
    const Result<QByteArray> contents = cdbEnvironmentScript(env, deviceEnvironment, workDir,
                                                             launch);
    QVERIFY(contents);
    const FilePath script = tempDir / "qtc-cdb-environment-test.cmd";
    QVERIFY(script.writeFileContents("@chcp 850 > nul\r\n@chcp\r\n" + *contents));
    const QScopeGuard removeScript([script] { script.removeFile(); });

    const Result<CommandLine> command = cdbDeviceCommandLine(script, cmdExe);
    QVERIFY(command);
    Process cmd;
    cmd.setCommand(*command);
    cmd.runBlocking(std::chrono::seconds(60));
    QCOMPARE(cmd.result(), ProcessResult::FinishedWithSuccess);
    const QStringList lines = cmd.cleanedStdOut().split('\n', Qt::SkipEmptyParts);
    QVERIFY(lines.contains("QTC_TEST_PERCENT=100% sure %NOPE"));
    QVERIFY(lines.contains("QTC_TEST_BANG=a!PATH!b"));
    QVERIFY(lines.contains("QTC_TEST_META=\"C:\\Program Files\";\"a&b|c<d>e(f)^"));
    // Read in 65001 and printed in 850. Read in 850, the UTF-8 bytes would come back as they are.
    QVERIFY(cmd.rawStdOut().contains("QTC_TEST_UMLAUT=Gr\x81\xe1" "e\r\n"));
    QVERIFY(cmd.rawStdOut().contains("\"QTC_ARG=a&b \x81\""));
    static const QRegularExpression codePageLine(": *\\d+\\.?$");
    const QStringList codePages = Utils::filtered(lines, [](const QString &line) {
        return !line.contains('=') && codePageLine.match(line).hasMatch();
    });
    QCOMPARE(codePages.size(), 2);
    QCOMPARE(codePages.at(0), codePages.at(1));
    const auto isSet = [&lines](const QString &key) {
        return Utils::anyOf(lines, [key](const QString &line) {
            return line.startsWith(key + '=', Qt::CaseInsensitive);
        });
    };
    QVERIFY(!isSet("USERNAME"));
    QVERIFY(!isSet("COMPUTERNAME"));
    QVERIFY(!isSet("QTC_CDB"));
    QVERIFY(!isSet("QTC_CHCP"));
    QVERIFY(lines.contains("PATH=" + quotedPathEntry, Qt::CaseInsensitive));
    QVERIFY(lines.contains(workDir.nativePath(), Qt::CaseInsensitive));
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

    QTest::newRow("drive letter")
        << debugStrings({"C:/Users/qt/work/qt/qtbase/src/corelib/global/qglobal.cpp"})
        << QStringList{"C:/Users/qt/work/qt"};

    QTest::newRow("drive root")
        << debugStrings({"c:/qtbase/src/corelib/global/qglobal.cpp"}) << QStringList();

    QTest::newRow("no drive letter")
        << debugStrings({"1:/qt/qtbase/src/corelib/global/qglobal.cpp",
                         "C:qt/qtbase/src/corelib/global/qglobal.cpp"})
        << QStringList();
}

void DebuggerUnitTests::testQtBuildSourceRoots()
{
    QFETCH(QByteArray, blob);
    QFETCH(QStringList, roots);

    QCOMPARE(qtBuildSourceRoots(blob), roots);
}

static QByteArray le32(quint32 value)
{
    QByteArray result(4, '\0');
    qToLittleEndian(value, result.data());
    return result;
}

// A PDB with just an info stream and a string table, whose blocks are out of
// order so that they have to be gathered, and partly consecutive.
static QByteArray fakePdb(const QByteArray &strings)
{
    const int blockSize = 512;
    const QByteArray streamNames("/LinkInfo\0/names\0", 17);
    QByteArray info(28, '\0'); // Version, signature, age, GUID.
    info += le32(streamNames.size()) + streamNames;
    info += le32(2) + le32(4);          // Entries, capacity.
    info += le32(1) + le32(0b11);       // Present buckets.
    info += le32(0);                    // Deleted buckets.
    info += le32(0) + le32(0);          // "/LinkInfo" is stream 0.
    info += le32(10) + le32(2);         // "/names" is stream 2.

    const QByteArray names = le32(0xeffeeffe) + le32(1) + le32(strings.size()) + strings
                             + le32(0);
    const QList<quint32> namesBlocks = {7, 5, 6};

    QByteArray directory = le32(3) + le32(0) + le32(info.size()) + le32(names.size());
    directory += le32(4);
    for (const quint32 block : namesBlocks)
        directory += le32(block);

    QList<QByteArray> blocks(8, QByteArray(blockSize, '\0'));
    const auto put = [&blocks](int block, const QByteArray &data) {
        blocks[block].replace(0, data.size(), data);
    };
    QByteArray super("Microsoft C/C++ MSF 7.00\r\n\x1a" "DS\0\0\0", 32);
    super += le32(blockSize) + le32(1) + le32(blocks.size()) + le32(directory.size());
    super += le32(0) + le32(2);
    put(0, super);
    put(2, le32(3));
    put(3, directory);
    put(4, info);
    for (int i = 0; i < namesBlocks.size(); ++i)
        put(namesBlocks.at(i), names.mid(i * blockSize, blockSize));
    return blocks.join();
}

void DebuggerUnitTests::testPdbSourceFileNames()
{
    QStringList files{"", "C:\\Users\\qt\\work\\qt\\qtbase\\src\\corelib\\global\\qglobal.cpp"};
    for (int i = 0; i < 25; ++i)
        files << QString("C:\\Users\\qt\\work\\qt\\qtbase_build\\gen\\file%1.h").arg(i);
    const QByteArray strings = debugStrings(files);
    // The string table has to take all three of its blocks.
    QVERIFY(strings.size() + 16 > 2 * 512 && strings.size() + 16 <= 3 * 512);

    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath pdb = FilePath::fromString(tmp.path()) / "Qt6Core.pdb";
    const QByteArray contents = fakePdb(strings);

    QVERIFY(pdb.writeFileContents(contents));
    QCOMPARE(pdbSourceFileNames(pdb), strings);
    QCOMPARE(qtBuildSourceRoots(pdbSourceFileNames(pdb).replace('\\', '/')),
             QStringList{"C:/Users/qt/work/qt"});

    // Anything but an intact PDB yields nothing.
    QVERIFY(pdb.writeFileContents(contents.first(4 * 512)));
    QCOMPARE(pdbSourceFileNames(pdb), QByteArray());

    QByteArray noMagic = contents;
    noMagic[0] = 'm';
    QVERIFY(pdb.writeFileContents(noMagic));
    QCOMPARE(pdbSourceFileNames(pdb), QByteArray());

    QByteArray badBlock = contents;
    qToLittleEndian<quint32>(100, badBlock.data() + 3 * 512 + 20);
    QVERIFY(pdb.writeFileContents(badBlock));
    QCOMPARE(pdbSourceFileNames(pdb), QByteArray());
}

struct FakeSection
{
    QByteArray name;
    QByteArray data;
    QByteArray padding;
};

// A PE file without optional header whose sections all have long names, which
// makes it need a string table. lld puts the names of the symbols first.
static QByteArray fakePe(const QList<FakeSection> &sections, qsizetype symbolNames = 0)
{
    QByteArray dos(64, '\0');
    dos[0] = 'M';
    dos[1] = 'Z';
    qToLittleEndian<quint32>(dos.size(), dos.data() + 0x3c);

    const int tableStart = dos.size() + 24;
    qsizetype dataStart = tableStart + sections.size() * 40;
    QByteArray table;
    QByteArray data;
    QByteArray names = le32(0) + QByteArray(symbolNames, 's');
    for (const FakeSection &section : sections) {
        QByteArray entry = "/" + QByteArray::number(names.size());
        entry.resize(8, '\0');
        names += section.name + '\0';
        entry += le32(section.data.size()) + le32(0);
        entry += le32(section.data.size() + section.padding.size());
        entry += le32(dataStart + data.size()) + QByteArray(16, '\0');
        table += entry;
        data += section.data + section.padding;
    }
    qToLittleEndian<quint32>(names.size(), names.data());

    QByteArray header = QByteArray("PE\0\0", 4) + QByteArray(20, '\0');
    qToLittleEndian<quint16>(0x8664, header.data() + 4);
    qToLittleEndian<quint16>(sections.size(), header.data() + 6);
    qToLittleEndian<quint32>(dataStart + data.size(), header.data() + 12);
    return dos + header + table + data + names;
}

void DebuggerUnitTests::testPeDebugStrings()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath dir = FilePath::fromString(tmp.path());
    const FilePath library = dir / "Qt6Core.dll";
    const FilePath companion = dir / "Qt6Core.debug";

    const QByteArray str = debugStrings({"C:/Users/qt/work/qt/qtbase/src/corelib/io"});
    const QByteArray lineStr = debugStrings({"C:\\Users\\qt\\work\\qt\\qtbase_build",
                                             "D:/qt/qtbase/src/corelib/qglobal.cpp"});
    // Past the end of the data, the padding up to the file alignment is no string.
    const QByteArray padding("/padding/qtbase/src/", 20);
    const QStringList roots{"C:/Users/qt/work/qt", "D:/qt"};

    QVERIFY(companion.writeFileContents(fakePe({{".debug_str", str, padding},
                                                {".debug_line_str", lineStr, padding}},
                                               512 * 1024)));
    QCOMPARE(qtBuildSourceRoots(peDebugStrings(companion).replace('\\', '/')), roots);

    // A stripped library leads to its companion.
    QVERIFY(library.writeFileContents(
        fakePe({{".text", "code", {}},
                {".gnu_debuglink", QByteArray("Qt6Core.debug\0\0\0", 16) + le32(0), {}}})));
    QCOMPARE(qtBuildSourceRoots(peDebugStrings(library).replace('\\', '/')), roots);

    QVERIFY(companion.removeFile());
    QCOMPARE(qtBuildSourceRoots(peDebugStrings(library)), QStringList());

    QVERIFY(library.writeFileContents(str));
    QCOMPARE(peDebugStrings(library), QByteArray(1, '\0'));
}

// The PDB of a Qt on a Windows device is read through the device's file access,
// piecewise at the offsets of its blocks.
void DebuggerUnitTests::testPdbSourceFileNamesOnDevice()
{
    const IDevicePtr device = addWindowsTestDevice();
    if (!device)
        QSKIP(noWindowsTestDevice);
    const QScopeGuard cleanup([id = device->id()] { DeviceManager::removeDevice(id); });
    QVERIFY2(connectToDevice(device), "Cannot connect to the device.");

    FilePath pdb;
    const FilePath qtRoot = device->rootPath().withNewPath("C:/Qt");
    const DirFilterFlags dirs = DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot;
    for (const FilePath &version : qtRoot.dirEntries(dirs)) {
        for (const FilePath &qt : version.dirEntries(FileFilter({"msvc*"}, dirs))) {
            if ((qt / "bin/Qt6Core.pdb").isReadableFile())
                pdb = qt / "bin/Qt6Core.pdb";
        }
    }
    if (pdb.isEmpty())
        QSKIP("The device has no MSVC Qt with a Qt6Core.pdb below C:/Qt.");

    QElapsedTimer timer;
    timer.start();
    const QByteArray names = pdbSourceFileNames(pdb);
    qDebug() << "Read the string table of" << pdb.toUserOutput() << "in" << timer.elapsed()
             << "ms:" << names.size() << "bytes";
    QVERIFY(!qtBuildSourceRoots(QByteArray(names).replace('\\', '/')).isEmpty());
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

    // What the QML debug service reports: its own spelling, and the model id
    // the request went out with.
    static GdbMi interpreterReport(int modelId, const QString &number)
    {
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
        add("modelid", QString::number(modelId));
        add("pending", "0");
        GdbMi list;
        list.m_type = GdbMi::List;
        list.addChild(bkpt);
        return list;
    }

    // Answer the request this breakpoint's op went out as. Picking the request
    // by model id rather than by recency keeps whatever else the session had
    // claimed out of it.
    bool answerAsInterpreter(int modelId, BreakpointOp op, const QString &number)
    {
        for (const BreakpointChangeRequest &request : std::as_const(requests)) {
            if (request.modelId == modelId && request.op == op) {
                emit breakpointEvent(request.requestId, op, true,
                                     interpreterReport(modelId, number));
                return true;
            }
        }
        return false;
    }

    void reportAsInterpreter(int modelId, const QString &number)
    {
        emit breakpointModified(interpreterReport(modelId, number));
    }

    QList<BreakpointChangeRequest> requests;
    // What the engine went on to read, which is how a view that was filled is
    // told from one that was left alone.
    QList<RefreshKind> refreshes;

    void start() override {}
    void shutdownInferior(ShutdownMode) override {}
    void shutdownEngine() override {}
    void execute(const ExecutionRequest &) override {}
    void refresh(const RefreshRequest &request) override { refreshes.append(request.kind); }
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

// The model is the session's own, so the breakpoint has to go again.
static GlobalBreakpoint createInterpreterBreakpoint()
{
    BreakpointParameters params;
    params.type = BreakpointByFileAndLine;
    params.fileName = FilePath::fromUserInput("Main.qml");
    params.textPosition = {10, -1};
    params.enabled = true;
    return BreakpointManager::createBreakpoint(params);
}

// The service spells an enabled breakpoint "1", gdb spells it "y". Read as gdb
// output, the reply to the insert turns the breakpoint off the moment it is
// made, and nothing the user does in the view can turn it back on, since the
// report answering the update is read the same way.
void DebuggerUnitTests::testInterpreterBreakpointStaysEnabled()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});

    const GlobalBreakpoint gbp = createInterpreterBreakpoint();
    QVERIFY(gbp);
    const QScopeGuard removeBreakpoint([gbp] { gbp->removeBreakpointFromModel(); });
    const int modelId = gbp->modelId();

    BreakpointManager::claimBreakpointsForEngine(engine);
    const Breakpoint bp = engine->breakHandler()->findBreakpointByModelId(modelId);
    QVERIFY(bp);

    QVERIFY(backend->answerAsInterpreter(modelId, BreakpointOp::Insert, "1"));
    QVERIFY2(bp->isEnabled(), "the reply to the insert turned the breakpoint off");
    QCOMPARE(bp->responseId(), QString("1"));

    // What enabling a breakpoint in the view comes down to: the service
    // reports the change, and reading that as gdb output turns it off again.
    bp->setEnabled(false);
    backend->reportAsInterpreter(modelId, "1");
    QVERIFY2(bp->isEnabled(), "the report of the change turned the breakpoint off");

    // A report left over from a breakpoint that is gone names a model id no
    // longer in the model. Its number is the interpreter's own counting and
    // says nothing about whose breakpoint it is, so nothing may be looked up
    // by it.
    bp->setCondition("x > 1");
    backend->reportAsInterpreter(modelId + 1000, "1");
    QVERIFY2(bp->isEnabled(), "a report for another breakpoint was applied by number");
    QCOMPARE(bp->condition(), QString("x > 1"));
}

// The debuggers match the file name case-sensitively. A breakpoint restored from
// a session that spells the file differently than the disk does is never hit.
// The disk is asked only once a debugger takes the breakpoint, so that restoring
// a session does not wait for a network share that does not answer.
void DebuggerUnitTests::testBreakpointFileNameSpelledAsOnDisk()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});

    // A breakpoint without a file, or with a bare file name, has no spelling on
    // disk to take over.
    BreakpointParameters function(BreakpointByFunction);
    function.functionName = "main";
    const GlobalBreakpoint functionGbp = BreakpointManager::createBreakpoint(function);
    QVERIFY(functionGbp);
    const QScopeGuard removeFunctionBreakpoint(
        [functionGbp] { functionGbp->removeBreakpointFromModel(); });
    const GlobalBreakpoint relativeGbp = createInterpreterBreakpoint();
    QVERIFY(relativeGbp);
    const QScopeGuard removeRelativeBreakpoint(
        [relativeGbp] { relativeGbp->removeBreakpointFromModel(); });
    BreakpointManager::claimBreakpointsForEngine(engine);
    QVERIFY(engine->breakHandler()->findBreakpointByModelId(functionGbp->modelId()));
    QCOMPARE(functionGbp->requestedParameters().fileName, FilePath());
    QVERIFY(engine->breakHandler()->findBreakpointByModelId(relativeGbp->modelId()));
    QCOMPARE(relativeGbp->requestedParameters().fileName.path(), QString("Main.qml"));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath onDisk = FilePath::fromString(dir.path()).pathAppended("Source.cpp");
    QVERIFY(onDisk.writeFileContents("int main() {}\n"));
    const FilePath misspelled = onDisk.parentDir().pathAppended("source.CPP");
    if (!QFileInfo::exists(misspelled.path()))
        QSKIP("The file system is case-sensitive.");

    BreakpointParameters params;
    params.type = BreakpointByFileAndLine;
    params.fileName = misspelled;
    params.textPosition = {1, -1};
    const GlobalBreakpoint gbp = BreakpointManager::createBreakpoint(params);
    QVERIFY(gbp);
    const QScopeGuard removeBreakpoint([gbp] { gbp->removeBreakpointFromModel(); });
    QCOMPARE(gbp->requestedParameters().fileName.fileName(), QString("source.CPP"));

    BreakpointManager::claimBreakpointsForEngine(engine);
    const Breakpoint bp = engine->breakHandler()->findBreakpointByModelId(gbp->modelId());
    QVERIFY(bp);
    QCOMPARE(bp->requestedParameters().fileName.fileName(), QString("Source.cpp"));
}

// Insertion and removal both say they are proceeding, and the state machine
// takes the answer only from there. An update that skips it leaves the
// breakpoint stuck, so the view never shows what the user asked for.
void DebuggerUnitTests::testBreakpointUpdateAnnouncesItIsProceeding()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});

    const GlobalBreakpoint gbp = createInterpreterBreakpoint();
    QVERIFY(gbp);
    const QScopeGuard removeBreakpoint([gbp] { gbp->removeBreakpointFromModel(); });
    const int modelId = gbp->modelId();

    BreakpointManager::claimBreakpointsForEngine(engine);
    const Breakpoint bp = engine->breakHandler()->findBreakpointByModelId(modelId);
    QVERIFY(bp);
    QVERIFY(backend->answerAsInterpreter(modelId, BreakpointOp::Insert, "1"));
    QCOMPARE(bp->state(), BreakpointInserted);

    engine->breakHandler()->requestBreakpointUpdate(bp);
    QCOMPARE(bp->state(), BreakpointUpdateProceeding);
}

static QString preCheckStepTests(Kit **kit = nullptr, Toolchain **toolchain = nullptr)
{
    const auto buildsForThisMachine = [](Kit *candidate) {
        if (!candidate->isValid())
            return false;
        Toolchain *toolchain = ToolchainKitAspect::cxxToolchain(candidate);
        return toolchain && toolchain->targetAbi() == Abi::hostAbi();
    };
    Kit *usableKit = Utils::findOr(KitManager::kits(), nullptr, buildsForThisMachine);
    if (!usableKit)
        return "no kit of this machine's own architecture to build with";
    Toolchain *usedToolchain = ToolchainKitAspect::cxxToolchain(usableKit);
    if (kit)
        *kit = usableKit;
    if (toolchain)
        *toolchain = usedToolchain;
    return {};
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

static QTemporaryDir makeTestDir()
{
    QTemporaryDir inBuild(QCoreApplication::applicationDirPath() + "/qtc_debugger_XXXXXX");
    if (inBuild.isValid())
        return inBuild;
    return QTemporaryDir();
}

// Builds that program and runs it up to a marked line, holding it there so a
// test can ask for a step and see where it ends up. It needs no project: a
// compiler and a kit to name the debugger are the whole of it.
class SteppingSession
{
public:
    explicit SteppingSession(const char *source = s_steppingSource,
                             const QStringList &extraFlags = {})
        : m_sourceText(source)
        , m_extraFlags(extraFlags)
    {
        // Nothing here can answer a dialog, and the engine puts one up for a
        // breakpoint it cannot place. A breakpoint that does not take shows
        // up as a session that never stops where it was told to.
        m_warnedAboutBreakpoints = settings().showUnsupportedBreakpointWarning();
        settings().showUnsupportedBreakpointWarning.setValue(false);
    }

    ~SteppingSession()
    {
        QObject::disconnect(m_stoppedConnection);
        settings().showUnsupportedBreakpointWarning.setValue(m_warnedAboutBreakpoints);
        if (m_runControl) {
            m_runControl->initiateStop();
            QTestEventLoop::instance().enterLoop(30);
        }
        if (m_breakpoint)
            m_breakpoint->deleteBreakpoint();
        for (const GlobalBreakpoint &other : std::as_const(m_disabledBreakpoints)) {
            if (other)
                other->setEnabled(true);
        }
    }

    // Empty when the session is up, otherwise why it is not.
    QString start(const QString &marker)
    {
        if (!m_dir.isValid())
            return "no temporary directory to build in";

        Kit *kit = nullptr;
        Toolchain *toolchain = nullptr;
        if (QString reason = preCheckStepTests(&kit, &toolchain); !reason.isEmpty())
            return reason;
        QTC_ASSERT(kit && toolchain, return "kit or toolchain invalid");
        if (m_gdbDap) {
            // gdb only grew its DAP mode along the way.
            if (DebuggerKitAspect::engineType(kit) != GdbEngineType)
                m_skipReason = "The debugger of the kit is not gdb.";
            else if (QVersionNumber::fromString(DebuggerKitAspect::version(kit))
                     < QVersionNumber(14, 0, 50))
                m_skipReason = "The gdb of the kit is too old for its DAP mode.";
            if (!m_skipReason.isEmpty())
                return m_skipReason;
        }

        const FilePath dir = FilePath::fromString(m_dir.path());
        m_source = dir / "stepping.cpp";
        m_executable = (dir / "stepping").withExecutableSuffix();
        if (!m_source.writeFileContents(m_sourceText))
            return "could not write " + m_source.toUserOutput();

        m_line = markerLine(marker);
        if (m_line <= 0)
            return "no line marked " + marker;

        Process compiler;
        if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID) {
            // cl needs the environment of its Visual Studio installation, and
            // its own spelling of what the gcc-style flags say.
            Environment env = Environment::systemEnvironment();
            toolchain->addToEnvironment(env);
            compiler.setEnvironment(env);
            compiler.setWorkingDirectory(dir);
            compiler.setCommand({toolchain->compilerCommand(),
                                 {"/nologo", "/Zi", "/Od", "/EHsc", "/std:c++17",
                                  m_source.nativePath(), "/Fe" + m_executable.nativePath(),
                                  "/link", "/DEBUG"}});
        } else {
            compiler.setCommand({toolchain->compilerCommand(),
                                 QStringList{"-g", "-O0", m_source.nativePath(),
                                             "-o", m_executable.nativePath()}
                                     + m_extraFlags});
        }
        compiler.runBlocking(std::chrono::seconds(60));
        if (compiler.exitCode() != 0 || !m_executable.isExecutableFile())
            return "the inferior would not build: " + compiler.allOutput().left(300);

        // The session the tests run in may hold breakpoints of its own, and
        // CDB takes one on a file of the same name elsewhere for this one.
        for (const GlobalBreakpoint &other : BreakpointManager::globalBreakpoints()) {
            if (other && other->isEnabled() && !m_keptBreakpoints.contains(other)) {
                other->setEnabled(false);
                m_disabledBreakpoints.append(other);
            }
        }

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
        if (m_gdbDap)
            rp.setCppEngineType(GdbDapEngineType);
        QObject::connect(m_runControl, &RunControl::stopped,
                         &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
        m_stoppedConnection = QObject::connect(m_runControl, &RunControl::stopped,
                                               m_runControl, [this] { m_stopped = true; });
        m_runControl->setRunRecipe(debuggerRecipe(m_runControl, rp));
        m_runControl->start();

        // EngineManager holds every engine the application ever made, and other
        // tests leave theirs behind, so take the one that was not there before.
        // A session that has already ended will not stop anywhere any more.
        const bool arrived = QTest::qWaitFor([this, &before] {
            for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
                if (candidate && !before.contains(candidate))
                    m_engine = candidate;
            }
            if (m_stopped)
                return true;
            // Stopped is not yet arrived: the stack the test reads about comes
            // in after the state does.
            return m_engine && m_engine->state() == InferiorStopOk
                   && m_engine->stackHandler()->currentFrame().line == m_line;
        }, 90000);
        if (!arrived || m_stopped) {
            QString why = "the session never stopped at " + marker;
            if (m_stopped)
                why += ", it ended first";
            if (m_engine) {
                why += ". The end of the log:\n"
                       + m_engine->logWindow()->combinedContents().right(3000);
            }
            return why;
        }
        return {};
    }

    DebuggerEngine *engine() const { return m_engine; }
    FilePath source() const { return m_source; }
    int line() const { return m_line; }
    FilePath sourceFile() const { return FilePath::fromString(m_dir.path()) / "stepping.cpp"; }
    // A breakpoint of the test's own, which start() leaves as it is.
    void keepBreakpoint(const GlobalBreakpoint &breakpoint) { m_keptBreakpoints.append(breakpoint); }

    // Runs the session with gdb's DAP mode instead of its machine interface.
    void useGdbDap() { m_gdbDap = true; }
    // Why start() did not get to a session that it could test.
    QString skipReason() const { return m_skipReason; }

    int markerLine(const QString &marker) const
    {
        const QStringList lines = QString::fromUtf8(m_sourceText).split('\n');
        for (int i = 0; i < lines.size(); ++i) {
            if (lines.at(i).contains(marker))
                return i + 1;
        }
        return 0;
    }

private:
    QByteArray m_sourceText;
    QStringList m_extraFlags;
    QTemporaryDir m_dir = makeTestDir();
    FilePath m_source;
    FilePath m_executable;
    RunControl *m_runControl = nullptr;
    QMetaObject::Connection m_stoppedConnection;
    bool m_stopped = false;
    QPointer<DebuggerEngine> m_engine;
    GlobalBreakpoint m_breakpoint;
    GlobalBreakpoints m_disabledBreakpoints;
    GlobalBreakpoints m_keptBreakpoints;
    bool m_warnedAboutBreakpoints = false;
    bool m_gdbDap = false;
    QString m_skipReason;
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
    Toolchain *toolchain = nullptr;
    if (const QString reason = preCheckStepTests(nullptr, &toolchain); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        QSKIP("MSVC puts the start of a function on its opening brace, which is where "
              "stepping in stops, not on the line after it.");

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
    if (const QString reason = preCheckStepTests(); !reason.isEmpty())
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
    if (const QString reason = preCheckStepTests(); !reason.isEmpty())
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

// Calls a debugger MCP tool and waits for its answer. Some tools wait for the
// debugger themselves, so the answer can take as long as a step does.
static Result<QJsonObject> callDebuggerTool(const QString &name, const QJsonObject &args = {})
{
    auto answer = std::make_shared<std::optional<Result<QJsonObject>>>();
    callMcpToolForTests(name, args, [answer](Result<QJsonObject> result) { *answer = result; });
    if (!QTest::qWaitFor([answer] { return answer->has_value(); }, 60000))
        return ResultError(QString("The tool \"%1\" never answered.").arg(name));
    return **answer;
}

// QVERIFY2 evaluates its message on success too.
static QString errorOf(const Result<QJsonObject> &result)
{
    return result ? QString() : result.error();
}

static int stopIdOf(const QJsonObject &result)
{
    return result.value("context").toObject().value("stop_id").toInt(-1);
}

static QString threadIdOf(const QJsonObject &result)
{
    return result.value("context").toObject().value("thread_id").toString();
}

static QJsonObject variableNamed(const QJsonObject &variables, const QString &name)
{
    for (const QJsonValue &v : variables.value("variables").toArray()) {
        if (v.toObject().value("name").toString() == name)
            return v.toObject();
    }
    return {};
}

// Puts either the engine's own implementation or the generic front end under
// test, and says why not if the environment has decided that already.
class BackendUnderTest
{
public:
    explicit BackendUnderTest(bool generic)
        : m_wasOn(commonSettings().useGenericDebugger())
    {
        if (isUseGenericDebuggerOverride() && useGenericDebuggerEnabled() != generic)
            m_reason = "QTC_USE_GENERIC_DEBUGGER decides which backends run.";
        commonSettings().useGenericDebugger.setValue(generic);
    }
    ~BackendUnderTest() { commonSettings().useGenericDebugger.setValue(m_wasOn); }

    QString reasonItIsNotUnderTest() const { return m_reason; }

private:
    bool m_wasOn = false;
    QString m_reason;
};

static void addBackendRows()
{
    QTest::addColumn<bool>("generic");
    QTest::newRow("engine") << false;
    QTest::newRow("generic") << true;
}

// The engines that report the threads of a stop one by one, after its stack.
static void addBackendRowsWithGdbDap()
{
    QTest::addColumn<bool>("generic");
    QTest::addColumn<bool>("gdbDap");
    QTest::newRow("engine") << false << false;
    QTest::newRow("generic") << true << false;
    QTest::newRow("gdb-dap") << false << true;
}

void DebuggerUnitTests::testMcpStepWaitsForTheNewStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpStepWaitsForTheNewStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> before = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(before, qPrintable(errorOf(before)));
    const int stopBefore = stopIdOf(*before);
    QVERIFY(stopBefore > 0);

    const Result<QJsonObject> step
        = callDebuggerTool("debugger_step_over", {{"wait_for_completion", true}});
    QVERIFY2(step, qPrintable(errorOf(step)));
    QCOMPARE(step->value("status").toString(), QString("completed"));
    QCOMPARE(stopIdOf(*step), stopBefore + 1);

    // Completed means the data is there: nothing more to wait for.
    const Result<QJsonObject> stack
        = callDebuggerTool("debugger_get_call_stack", {{"timeout_ms", 0}});
    QVERIFY2(stack, qPrintable(errorOf(stack)));
    QCOMPARE(stopIdOf(*stack), stopBefore + 1);
    const QJsonObject top = stack->value("frames").toArray().first().toObject();
    QCOMPARE(top.value("line").toInt(), session.markerLine("MARKER: after-call"));

    const Result<QJsonObject> variables
        = callDebuggerTool("debugger_get_variables", {{"timeout_ms", 0}});
    QVERIFY2(variables, qPrintable(errorOf(variables)));
    QCOMPARE(stopIdOf(*variables), stopBefore + 1);
    QCOMPARE(variableNamed(*variables, "second").value("value").toString(), QString("2"));
}

void DebuggerUnitTests::testMcpOperationIsNotCompleteBeforeTheStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpOperationIsNotCompleteBeforeTheStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // These answer without returning to the event loop, so the step cannot
    // have finished in between. Should one not answer right away, its answer
    // comes after this function has returned.
    const auto step = std::make_shared<std::optional<Result<QJsonObject>>>();
    callMcpToolForTests("debugger_step_over", {},
                        [step](Result<QJsonObject> result) { *step = result; });
    QVERIFY(step->has_value() && **step);
    const QString status = (**step)->value("status").toString();
    QVERIFY2(status == "accepted" || status == "in_progress", qPrintable(status));

    const auto early = std::make_shared<std::optional<Result<QJsonObject>>>();
    callMcpToolForTests("debugger_get_call_stack", {{"timeout_ms", 0}},
                        [early](Result<QJsonObject> result) { *early = result; });
    QVERIFY(early->has_value());
    QVERIFY2(!**early, "the stack of the stop before the step was presented as current");

    const int id = (**step)->value("operation_id").toInt();
    const Result<QJsonObject> waited
        = callDebuggerTool("debugger_wait_for_operation", {{"operation_id", id}});
    QVERIFY2(waited, qPrintable(errorOf(waited)));
    QCOMPARE(waited->value("status").toString(), QString("completed"));
    QCOMPARE(waited->value("operation_id").toInt(), id);
}

void DebuggerUnitTests::testMcpResumeThatFailsKeepsTheStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpResumeThatFailsKeepsTheStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
    // gdb has no frame above main to finish into, the others step out into
    // the runtime that called it.
    if (!session.engine()->debuggerName().startsWith("GDB"))
        QSKIP("Only gdb refuses to step out of main.");

    const Result<QJsonObject> before = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(before, qPrintable(errorOf(before)));

    const Result<QJsonObject> step
        = callDebuggerTool("debugger_step_out", {{"wait_for_completion", true}});
    QVERIFY2(step, qPrintable(errorOf(step)));
    QCOMPARE(step->value("status").toString(), QString("failed"));

    const Result<QJsonObject> after
        = callDebuggerTool("debugger_get_call_stack", {{"timeout_ms", 0}});
    QVERIFY2(after, qPrintable(errorOf(after)));
    QCOMPARE(stopIdOf(*after), stopIdOf(*before));
}

void DebuggerUnitTests::testMcpSelectFrameWaitsForItsLocals_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpSelectFrameWaitsForItsLocals()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> inCallee = callDebuggerTool("debugger_get_variables");
    QVERIFY2(inCallee, qPrintable(errorOf(inCallee)));
    QVERIFY(!variableNamed(*inCallee, "value").isEmpty());

    const Result<QJsonObject> select
        = callDebuggerTool("debugger_select_frame", {{"level", 1}, {"wait_for_completion", true}});
    QVERIFY2(select, qPrintable(errorOf(select)));
    QCOMPARE(select->value("status").toString(), QString("completed"));
    QCOMPARE(select->value("context").toObject().value("frame_level").toInt(), 1);

    const Result<QJsonObject> inCaller
        = callDebuggerTool("debugger_get_variables", {{"timeout_ms", 0}});
    QVERIFY2(inCaller, qPrintable(errorOf(inCaller)));
    QCOMPARE(inCaller->value("context").toObject().value("frame_level").toInt(), 1);
    QVERIFY(!variableNamed(*inCaller, "first").isEmpty());
    QVERIFY(variableNamed(*inCaller, "value").isEmpty());
}

void DebuggerUnitTests::testMcpSelectFrameBeforeTheStopsLocals_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpSelectFrameBeforeTheStopsLocals()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // The locals of frame 0 can still be on their way, and when they arrive,
    // frame 1 is current. They must not pass for the locals of frame 1.
    McpSessionState *state = McpSessionState::forEngine(session.engine());
    WatchHandler *watch = session.engine()->watchHandler();
    QStringList passedForCurrent;
    QObject receiver;
    QObject::connect(watch->model(), &WatchModelBase::updateFinished, &receiver, [&] {
        if (state->isLocalsReady() && !watch->findItem("local.first"))
            passedForCurrent.append(state->context().value("frame_level").toVariant().toString());
    });

    const Result<QJsonObject> select
        = callDebuggerTool("debugger_select_frame", {{"level", 1}, {"wait_for_completion", true}});
    QVERIFY2(select, qPrintable(errorOf(select)));
    QCOMPARE(select->value("status").toString(), QString("completed"));
    QVERIFY2(passedForCurrent.isEmpty(),
             qPrintable("locals without \"first\" passed for frame "
                        + passedForCurrent.join(", ")));

    const Result<QJsonObject> inCaller
        = callDebuggerTool("debugger_get_variables", {{"timeout_ms", 0}});
    QVERIFY2(inCaller, qPrintable(errorOf(inCaller)));
    QCOMPARE(inCaller->value("context").toObject().value("frame_level").toInt(), 1);
    QVERIFY(!variableNamed(*inCaller, "first").isEmpty());
}

void DebuggerUnitTests::testLocalsOfAFrameSelectedEarlyKeepNothingOfTheOtherFrame_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testLocalsOfAFrameSelectedEarlyKeepNothingOfTheOtherFrame()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    WatchHandler *watch = session.engine()->watchHandler();
    int started = 0;
    int finished = 0;
    QObject receiver;
    QObject::connect(watch->model(), &WatchModelBase::updateStarted, &receiver, [&started] {
        ++started;
    });
    QObject::connect(watch->model(), &WatchModelBase::updateFinished, &receiver, [&finished] {
        ++finished;
    });

    // The locals of frame 0 can still be on their way, and arrive after
    // frame 1 has asked for its own.
    session.engine()->activateFrame(1);
    QVERIFY(QTest::qWaitFor([&] {
        return started > 0 && finished >= started && watch->findItem("local.first");
    }, 30000));
    QVERIFY2(!watch->findItem("local.value"), "a local of frame 0 is shown for frame 1");
}

void DebuggerUnitTests::testMcpLocalsOutlastFramesAddedToTheStack_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpLocalsOutlastFramesAddedToTheStack()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> before = callDebuggerTool("debugger_get_variables");
    QVERIFY2(before, qPrintable(errorOf(before)));

    // As "Load QML Stack" does: the same stop and the same frame, in another row.
    StackFrame qmlFrame;
    qmlFrame.function = "onClicked";
    session.engine()->stackHandler()->prependFrames({qmlFrame});

    const Result<QJsonObject> after
        = callDebuggerTool("debugger_get_variables", {{"timeout_ms", 0}});
    QVERIFY2(after, qPrintable(errorOf(after)));
    QCOMPARE(stopIdOf(*after), stopIdOf(*before));
    QVERIFY(!variableNamed(*after, "value").isEmpty());
}

// A second thread that stops while the first one waits for it.
static const char s_threadSource[] = R"CPP(
#include <thread>

static int twice(int value)
{
    int doubled = value * 2; // MARKER: in-thread
    return doubled;
}

int main()
{
    int result = 0;
    int start = 1; // MARKER: before-thread
    std::thread worker([&result, start] { result = twice(start); });
    worker.join();
    return result - 2;
}
)CPP";

void DebuggerUnitTests::testMcpStackFollowsTheThreadThatStopped_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpStackFollowsTheThreadThatStopped()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_threadSource, {"-pthread"});
    const QString problem = session.start("MARKER: before-thread");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> inMain = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(inMain, qPrintable(errorOf(inMain)));
    const QString mainThread = threadIdOf(*inMain);
    QVERIFY(!mainThread.isEmpty());

    // The stop is in a thread other than the one of the stop before.
    const int workerLine = session.markerLine("MARKER: in-thread");
    const Result<QJsonObject> run
        = callDebuggerTool("debugger_run_to_line", {{"file", session.source().path()},
                                                    {"line", workerLine},
                                                    {"wait_for_completion", true}});
    QVERIFY2(run, qPrintable(errorOf(run)));
    QVERIFY2(run->value("status").toString() == "completed",
             QJsonDocument(*run).toJson(QJsonDocument::Compact).constData());
    const QString workerThread = threadIdOf(*run);
    QVERIFY(!workerThread.isEmpty());
    QVERIFY(workerThread != mainThread);

    const Result<QJsonObject> inWorker
        = callDebuggerTool("debugger_get_call_stack", {{"timeout_ms", 0}});
    QVERIFY2(inWorker, qPrintable(errorOf(inWorker)));
    QCOMPARE(threadIdOf(*inWorker), workerThread);
    const QJsonObject top = inWorker->value("frames").toArray().first().toObject();
    QCOMPARE(top.value("line").toInt(), workerLine);

    const Result<QJsonObject> select
        = callDebuggerTool("debugger_select_thread",
                           {{"id", mainThread}, {"wait_for_completion", true}});
    QVERIFY2(select, qPrintable(errorOf(select)));
    QCOMPARE(select->value("status").toString(), QString("completed"));
    QCOMPARE(threadIdOf(*select), mainThread);

    const Result<QJsonObject> backInMain
        = callDebuggerTool("debugger_get_call_stack", {{"timeout_ms", 0}});
    QVERIFY2(backInMain, qPrintable(errorOf(backInMain)));
    QCOMPARE(threadIdOf(*backInMain), mainThread);
    for (const QJsonValue &frame : backInMain->value("frames").toArray())
        QVERIFY(!frame.toObject().value("function").toString().startsWith("twice"));
}

// A local whose members are fetched only when asked for.
static const char s_structSource[] = R"CPP(
struct Pair { int first; int second; };

int main()
{
    Pair pair{1, 2};
    int sum = pair.first + pair.second; // MARKER: struct
    return sum;
}
)CPP";

void DebuggerUnitTests::testMcpVariableFetchesItsChildren_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpVariableFetchesItsChildren()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_structSource);
    const QString problem = session.start("MARKER: struct");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> locals = callDebuggerTool("debugger_get_variables");
    QVERIFY2(locals, qPrintable(errorOf(locals)));
    const QJsonObject pair = variableNamed(*locals, "pair");
    QVERIFY2(pair.value("has_children").toBool() && !pair.contains("children"),
             QJsonDocument(pair).toJson(QJsonDocument::Compact).constData());

    const Result<QJsonObject> fetched = callDebuggerTool(
        "debugger_get_variable", {{"iname", pair.value("iname")}});
    QVERIFY2(fetched, qPrintable(errorOf(fetched)));
    const QJsonArray children = fetched->value("variable").toObject().value("children").toArray();
    QCOMPARE(children.size(), 2);
    QCOMPARE(children.at(1).toObject().value("value").toString(), QString("2"));
}

// Two threads that wait in a function of their own name while main stops.
static const char s_threadsSource[] = R"CPP(
#include <atomic>
#include <chrono>
#include <thread>

std::atomic<int> started{0};

void parkedWorker()
{
    ++started;
    for (;;)
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // MARKER: parked
}

int main()
{
    std::thread first(parkedWorker);
    std::thread second(parkedWorker);
    while (started < 2)
        std::this_thread::yield();
    int both = started; // MARKER: all-started
    // Long enough for a worker to run into a breakpoint set at this stop.
    std::this_thread::sleep_for(std::chrono::seconds(30));
    first.detach();
    second.detach();
    return both - 2;
}
)CPP";

static bool hasFrameIn(const QJsonObject &thread, const QString &function)
{
    for (const QJsonValue &frame : thread.value("frames").toArray()) {
        if (frame.toObject().value("function").toString().startsWith(function))
            return true;
    }
    return false;
}

void DebuggerUnitTests::testMcpCollectsTheStacksOfAllThreads_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpCollectsTheStacksOfAllThreads()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_threadsSource, {"-std=c++17", "-pthread"});
    const QString problem = session.start("MARKER: all-started");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> before = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(before, qPrintable(errorOf(before)));
    const QString originalThread = before->value("context").toObject().value("thread_id").toString();
    QVERIFY(!originalThread.isEmpty());

    const Result<QJsonObject> stacks
        = callDebuggerTool("debugger_get_thread_stacks", {{"max_frames", 10}});
    QVERIFY2(stacks, qPrintable(errorOf(stacks)));
    QVERIFY(stacks->value("same_stop").toBool());
    QVERIFY(!stacks->value("target_resumed").toBool());
    QVERIFY(stacks->value("selection_restored").toBool());
    QCOMPARE(stacks->value("stop_id").toInt(), stopIdOf(*before));

    int workers = 0;
    int mains = 0;
    const QJsonArray threads = stacks->value("threads").toArray();
    for (const QJsonValue &value : threads) {
        const QJsonObject thread = value.toObject();
        QVERIFY2(!thread.contains("error"),
                 qPrintable(thread.value("id").toString() + ": "
                            + thread.value("error").toString()));
        if (hasFrameIn(thread, "parkedWorker"))
            ++workers;
        const QJsonObject top = thread.value("frames").toArray().first().toObject();
        if (top.value("function").toString().startsWith("main")) {
            ++mains;
            QCOMPARE(top.value("line").toInt(), session.line());
            QCOMPARE(thread.value("id").toString(), originalThread);
        }
    }
    QVERIFY2(threads.size() >= 3, qPrintable(QString::number(threads.size())));
    QCOMPARE(workers, 2);
    QCOMPARE(mains, 1);

    const Result<QJsonObject> after = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(after, qPrintable(errorOf(after)));
    QCOMPARE(after->value("context").toObject().value("thread_id").toString(), originalThread);
    QCOMPARE(stopIdOf(*after), stopIdOf(*before));
    QCOMPARE(after->value("frames").toArray().first().toObject().value("line").toInt(),
             session.line());
}

void DebuggerUnitTests::testMcpStackOfAStopInAnotherThread_data()
{
    addBackendRowsWithGdbDap();
}

void DebuggerUnitTests::testMcpStackOfAStopInAnotherThread()
{
    QFETCH(bool, generic);
    QFETCH(bool, gdbDap);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_threadsSource, {"-std=c++17", "-pthread"});
    if (gdbDap)
        session.useGdbDap();
    const QString problem = session.start("MARKER: all-started");
    if (!session.skipReason().isEmpty())
        QSKIP(qPrintable(session.skipReason()));
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
    const Result<QJsonObject> inMain = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(inMain, qPrintable(errorOf(inMain)));
    const QString mainThread = inMain->value("context").toObject().value("thread_id").toString();

    // The next stop is in a worker, so the thread that stops is not the one
    // that was selected.
    BreakpointParameters params;
    params.type = BreakpointByFileAndLine;
    params.fileName = session.sourceFile();
    params.textPosition = {session.markerLine("MARKER: parked"), -1};
    params.enabled = true;
    const GlobalBreakpoint parked = BreakpointManager::createBreakpoint(params);
    QVERIFY(parked);
    const QScopeGuard remove([parked] {
        if (parked)
            parked->deleteBreakpoint();
    });

    const Result<QJsonObject> resumed
        = callDebuggerTool("debugger_continue", {{"wait_for_completion", true}});
    QVERIFY2(resumed, qPrintable(errorOf(resumed)));
    QVERIFY2(resumed->value("status").toString() == "completed",
             QJsonDocument(*resumed).toJson(QJsonDocument::Compact).constData());

    const Result<QJsonObject> inWorker = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(inWorker, qPrintable(errorOf(inWorker)));
    QVERIFY(inWorker->value("context").toObject().value("thread_id").toString() != mainThread);
    QVERIFY2(hasFrameIn(*inWorker, "parkedWorker"),
             QJsonDocument(*inWorker).toJson(QJsonDocument::Compact).constData());
}

// A worker that stays in this file, so that its innermost frame has a source.
static const char s_spinningThreadSource[] = R"CPP(
#include <atomic>
#include <thread>

std::atomic<bool> done{false};
std::atomic<int> started{0};

void spinningWorker()
{
    ++started;
    while (!done) {
    } // MARKER: spinning
}

int main()
{
    std::thread worker(spinningWorker);
    while (started < 1)
        std::this_thread::yield();
    int one = started; // MARKER: worker-started
    done = true;
    worker.join();
    return one - 1;
}
)CPP";

void DebuggerUnitTests::testMcpSelectThreadWaitsForItsStack_data()
{
    addBackendRowsWithGdbDap();
}

void DebuggerUnitTests::testMcpSelectThreadWaitsForItsStack()
{
    QFETCH(bool, generic);
    QFETCH(bool, gdbDap);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_spinningThreadSource, {"-std=c++17", "-pthread"});
    if (gdbDap)
        session.useGdbDap();
    const QString problem = session.start("MARKER: worker-started");
    if (!session.skipReason().isEmpty())
        QSKIP(qPrintable(session.skipReason()));
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> inMain = callDebuggerTool("debugger_get_call_stack");
    QVERIFY2(inMain, qPrintable(errorOf(inMain)));
    const QString current = inMain->value("context").toObject().value("thread_id").toString();
    QStringList others;
    session.engine()->threadsHandler()->forItemsAtLevel<1>([&](ThreadItem *thread) {
        if (thread->id() != current)
            others.append(thread->id());
    });
    QVERIFY(!others.isEmpty());

    // Windows starts threads of its own in every process, so the worker is
    // the one of the others that has spinningWorker() on its stack.
    QStringList workers;
    for (const QString &other : std::as_const(others)) {
        const Result<QJsonObject> selected = callDebuggerTool(
            "debugger_select_thread", {{"id", other}, {"wait_for_completion", true}});
        QVERIFY2(selected, qPrintable(errorOf(selected)));
        QCOMPARE(selected->value("status").toString(), QString("completed"));

        const Result<QJsonObject> stack
            = callDebuggerTool("debugger_get_call_stack", {{"timeout_ms", 0}});
        QVERIFY2(stack, qPrintable(errorOf(stack)));
        QCOMPARE(stack->value("context").toObject().value("thread_id").toString(), other);
        if (hasFrameIn(*stack, "spinningWorker"))
            workers.append(other);
    }
    QVERIFY2(workers.size() == 1,
             qPrintable("spinningWorker() is on the stack of " + workers.join(", ")
                        + " of " + others.join(", ")));
}

void DebuggerUnitTests::testMcpThreadStacksAreBounded_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpThreadStacksAreBounded()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session(s_threadsSource, {"-std=c++17", "-pthread"});
    const QString problem = session.start("MARKER: all-started");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> shallow = callDebuggerTool(
        "debugger_get_thread_stacks", {{"max_frames", 1}, {"fields", QJsonArray{"function"}}});
    QVERIFY2(shallow, qPrintable(errorOf(shallow)));
    for (const QJsonValue &value : shallow->value("threads").toArray()) {
        const QJsonObject thread = value.toObject();
        const QJsonArray frames = thread.value("frames").toArray();
        QCOMPARE(frames.size(), 1);
        const QJsonObject frame = frames.first().toObject();
        QVERIFY(frame.contains("function"));
        QVERIFY(!frame.contains("line"));
        QVERIFY(!frame.contains("address"));
        if (thread.value("frames_loaded").toInt() > 1)
            QVERIFY(thread.value("truncated").toBool());
    }

    const Result<QJsonObject> one
        = callDebuggerTool("debugger_get_thread_stacks", {{"max_threads", 1}});
    QVERIFY2(one, qPrintable(errorOf(one)));
    QCOMPARE(one->value("threads").toArray().size(), 1);
    QVERIFY(one->value("omitted_threads").toArray().size() >= 2);
}

static QString programCounterName()
{
    return Abi::hostAbi().architecture() == Abi::ArmArchitecture ? QString("pc") : QString("rip");
}

static QString stackPointerName()
{
    return Abi::hostAbi().architecture() == Abi::ArmArchitecture ? QString("sp") : QString("rsp");
}

static QJsonObject registerNamed(const QJsonObject &registers, const QString &name)
{
    for (const QJsonValue &r : registers.value("registers").toArray()) {
        if (r.toObject().value("name").toString() == name)
            return r.toObject();
    }
    return {};
}

static quint64 hexValue(const QJsonValue &value)
{
    return value.toString().toULongLong(nullptr, 16);
}

void DebuggerUnitTests::testMcpReadsTheRegistersOfTheCurrentStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpReadsTheRegistersOfTheCurrentStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const QString pc = programCounterName();
    for (int step = 0; step < 2; ++step) {
        if (step == 1) {
            const Result<QJsonObject> stepped
                = callDebuggerTool("debugger_step_over", {{"wait_for_completion", true}});
            QVERIFY2(stepped, qPrintable(errorOf(stepped)));
            QCOMPARE(stepped->value("status").toString(), QString("completed"));
        }
        const Result<QJsonObject> stack = callDebuggerTool("debugger_get_call_stack");
        QVERIFY2(stack, qPrintable(errorOf(stack)));
        const quint64 topAddress = hexValue(
            stack->value("frames").toArray().first().toObject().value("address"));
        QVERIFY(topAddress != 0);

        const Result<QJsonObject> registers = callDebuggerTool("debugger_get_registers");
        QVERIFY2(registers, qPrintable(errorOf(registers)));
        QCOMPARE(stopIdOf(*registers), stopIdOf(*stack));
        QVERIFY(registers->value("live_cpu_values").toBool());

        const QJsonObject programCounter = registerNamed(*registers, pc);
        QVERIFY2(programCounter.value("available").toBool(),
                 qPrintable(programCounter.value("reason").toString()));
        QCOMPARE(hexValue(programCounter.value("value")), topAddress);

        const QJsonObject stackPointer = registerNamed(*registers, stackPointerName());
        QVERIFY(stackPointer.value("available").toBool());
        QCOMPARE(stackPointer.value("bit_width").toInt(), 64);
        // All 64 bits, padded: "0x" and 16 digits.
        QCOMPARE(stackPointer.value("value").toString().size(), 18);
    }

    const Result<QJsonObject> selected = callDebuggerTool(
        "debugger_get_registers", {{"names", QJsonArray{pc, "no_such_register"}}});
    QVERIFY2(selected, qPrintable(errorOf(selected)));
    QCOMPARE(selected->value("registers").toArray().size(), 1);
    QCOMPARE(selected->value("unknown_names").toArray(), QJsonArray{"no_such_register"});
}

void DebuggerUnitTests::testMcpRegistersOfAnOuterFrame_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpRegistersOfAnOuterFrame()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: in-callee");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> inner = callDebuggerTool("debugger_get_registers");
    QVERIFY2(inner, qPrintable(errorOf(inner)));
    const quint64 innerPc = hexValue(registerNamed(*inner, programCounterName()).value("value"));

    const Result<QJsonObject> select
        = callDebuggerTool("debugger_select_frame", {{"level", 1}, {"wait_for_completion", true}});
    QVERIFY2(select, qPrintable(errorOf(select)));

    const Result<QJsonObject> outer = callDebuggerTool("debugger_get_registers");
    QVERIFY2(outer, qPrintable(errorOf(outer)));
    QCOMPARE(outer->value("frame_level").toInt(), 1);
    QVERIFY(outer->contains("note"));
    const quint64 outerPc = hexValue(registerNamed(*outer, programCounterName()).value("value"));
    QVERIFY(outerPc != 0);
    // Whichever frame the debugger reports the values for, it has to say so.
    QVERIFY2(outer->contains("values_for_frame_level"),
             "the debuggers under test are known to say which frame they report");
    if (outer->value("values_for_frame_level").toInt() == 1) {
        QVERIFY(!outer->value("live_cpu_values").toBool());
        QVERIFY(outerPc != innerPc);
    } else {
        QCOMPARE(outer->value("values_for_frame_level").toInt(), 0);
        QVERIFY(outer->value("live_cpu_values").toBool());
        QCOMPARE(outerPc, innerPc);
    }
}

void DebuggerUnitTests::testGdbRegisterValueAsHex_data()
{
    QTest::addColumn<QString>("value");
    QTest::addColumn<QString>("hex");

    QTest::newRow("integer") << "0x7fffffffe0b0" << "0x7fffffffe0b0";
    QTest::newRow("unreadable") << "<error reading variable>" << "";
    QTest::newRow("xmm")
        << "{v8_bfloat16 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, "
           "v4_float = {0x1, 0x2, 0x3, 0x4}, v16_int8 = {0x0 <repeats 16 times>}, "
           "v4_int32 = {0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c}, "
           "v2_int64 = {0x0706050403020100, 0x0f0e0d0c0b0a0908}, "
           "uint128 = 0x0f0e0d0c0b0a09080706050403020100}"
        << "0x0f0e0d0c0b0a09080706050403020100";
    QTest::newRow("ymm")
        << "{v16_bfloat16 = {0x0 <repeats 16 times>}, v8_float = {0x0, 0x0, 0x0, 0x0, "
           "0x0, 0x0, 0x0, 0x0}, v32_int8 = {0x0 <repeats 32 times>}, "
           "v8_int32 = {0x03020100, 0x07060504, 0x0b0a0908, 0x0f0e0d0c, 0x13121110, "
           "0x17161514, 0x1b1a1918, 0x1f1e1d1c}, v2_int128 = {0x0, 0x0}}"
        << "0x1f1e1d1c1b1a191817161514131211100f0e0d0c0b0a09080706050403020100";
    QTest::newRow("zmm-zero")
        << "{v32_bfloat16 = {0x0 <repeats 32 times>}, v16_float = {0x0 <repeats 16 times>}, "
           "v64_int8 = {0x0 <repeats 64 times>}, v16_int32 = {0x00000000 <repeats 16 times>}, "
           "v8_int64 = {0x0 <repeats 8 times>}, v4_int128 = {0x0, 0x0, 0x0, 0x0}}"
        << "0x" + QString(128, '0');
    QTest::newRow("zmm-run-in-low-lanes")
        << "{v16_float = {0x0 <repeats 16 times>}, "
           "v16_int32 = {0x00000000 <repeats 12 times>, 0x11111111, 0x22222222, 0x33333333, "
           "0x44444444}, v8_int64 = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2222222211111111, "
           "0x4444444433333333}}"
        << "0x44444444333333332222222211111111" + QString(96, '0');
}

void DebuggerUnitTests::testGdbRegisterValueAsHex()
{
    QFETCH(QString, value);
    QFETCH(QString, hex);

    QCOMPARE(gdbRegisterValueAsHex(value), hex);
}

void DebuggerUnitTests::testMemoryRetryParts()
{
    // Within one page there is nothing to retry.
    QVERIFY(memoryRetryParts(0x1000, 0, 16).isEmpty());
    QVERIFY(memoryRetryParts(0x1ff0, 0, 16).isEmpty());
    // A read across pages is retried page by page.
    const QList<QPair<quint64, quint64>> parts = memoryRetryParts(0x1ff0, 0, 0x2020);
    const QList<QPair<quint64, quint64>> expected{{0, 0x10}, {0x10, 0x1000}, {0x1010, 0x1000},
                                                  {0x2010, 0x10}};
    QCOMPARE(parts, expected);
    // The offset is into the read, the pages are of the address space.
    QCOMPARE(memoryRetryParts(0x1000, 0xff8, 16),
             (QList<QPair<quint64, quint64>>{{0xff8, 8}, {0x1000, 8}}));

    QCOMPARE(memoryErrorWithoutAddress("Cannot access memory at address 0x5"),
             QString("Cannot access memory"));
    QCOMPARE(memoryErrorWithoutAddress("Cannot access memory at address 0x7ffe0a1b"),
             memoryErrorWithoutAddress("Cannot access memory at address 0x7ffe0a1c"));
}

// Puts a pattern into ymm0 whose bytes say which byte they are.
static const char s_vectorSource[] = R"CPP(
alignas(32) static const unsigned char pattern[32] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

int main()
{
    asm volatile("vmovdqu %0, %%ymm0" : : "m"(pattern) : "xmm0");
    int done = 1; // MARKER: loaded
    return done - 1;
}
)CPP";

void DebuggerUnitTests::testMcpReadsAVectorRegister_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpReadsAVectorRegister()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    if (Abi::hostAbi().architecture() != Abi::X86Architecture || Abi::hostAbi().wordWidth() != 64)
        QSKIP("ymm0 is a register of x86-64.");
    Toolchain *toolchain = nullptr;
    if (const QString reason = preCheckStepTests(nullptr, &toolchain); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        QSKIP("The program loads ymm0 with inline assembly, which MSVC has none of on x64.");

    SteppingSession session(s_vectorSource);
    const QString problem = session.start("MARKER: loaded");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> registers
        = callDebuggerTool("debugger_get_registers", {{"names", QJsonArray{"ymm0"}}});
    QVERIFY2(registers, qPrintable(errorOf(registers)));
    const QJsonObject ymm0 = registerNamed(*registers, "ymm0");
    QVERIFY2(ymm0.value("available").toBool(),
             QJsonDocument(ymm0).toJson(QJsonDocument::Compact).constData());
    QCOMPARE(ymm0.value("bit_width").toInt(), 256);
    // Byte 31 is the most significant one.
    QString expected = "0x";
    for (int i = 31; i >= 0; --i)
        expected += QString("%1").arg(i, 2, 16, QLatin1Char('0'));
    QCOMPARE(ymm0.value("value").toString(), expected);
}

void DebuggerUnitTests::testMcpMemoryAccessPolicy_data()
{
    QTest::addColumn<int>("access");
    QTest::addColumn<int>("startMode");
    QTest::addColumn<int>("os");
    QTest::addColumn<QString>("ranges");
    QTest::addColumn<quint64>("address");
    QTest::addColumn<quint64>("length");
    QTest::addColumn<QString>("grant"); // Empty: refused.

    const int processes = CommonSettings::McpMemoryOfProcesses;
    QTest::newRow("local process")
        << processes << int(StartExternal) << int(Abi::DarwinOS) << QString()
        << quint64(0x1000) << quint64(16) << QString("process_memory");
    QTest::newRow("gdbserver on linux")
        << processes << int(AttachToRemoteServer) << int(Abi::LinuxOS) << QString()
        << quint64(0x1000) << quint64(16) << QString("process_memory");
    QTest::newRow("bare metal without ranges")
        << processes << int(AttachToRemoteServer) << int(Abi::BareMetalOS) << QString()
        << quint64(0x20000000) << quint64(16) << QString();
    QTest::newRow("bare metal in a range")
        << processes << int(AttachToRemoteServer) << int(Abi::BareMetalOS)
        << QString("0x08000000-0x080fffff, 0x20000000-0x2001ffff")
        << quint64(0x20000000) << quint64(0x20000) << QString("allowed_range");
    QTest::newRow("bare metal past the end of a range")
        << processes << int(AttachToRemoteServer) << int(Abi::BareMetalOS)
        << QString("0x20000000-0x2001ffff")
        << quint64(0x2001fff0) << quint64(32) << QString();
    QTest::newRow("unknown target")
        << processes << int(AttachToRemoteServer) << int(Abi::UnknownOS) << QString()
        << quint64(0x1000) << quint64(16) << QString();
    QTest::newRow("bare metal even when launched")
        << processes << int(StartInternal) << int(Abi::BareMetalOS) << QString()
        << quint64(0x1000) << quint64(16) << QString();
    QTest::newRow("turned off")
        << int(CommonSettings::McpMemoryNever) << int(StartExternal) << int(Abi::DarwinOS)
        << QString() << quint64(0x1000) << quint64(16) << QString();
    QTest::newRow("anywhere")
        << int(CommonSettings::McpMemoryAnywhere) << int(AttachToRemoteServer)
        << int(Abi::BareMetalOS) << QString() << quint64(0x40000000) << quint64(4)
        << QString("any_memory");
}

void DebuggerUnitTests::testMcpMemoryAccessPolicy()
{
    QFETCH(int, access);
    QFETCH(int, startMode);
    QFETCH(int, os);
    QFETCH(QString, ranges);
    QFETCH(quint64, address);
    QFETCH(quint64, length);
    QFETCH(QString, grant);

    CommonSettings &s = commonSettings();
    const int oldAccess = s.mcpMemoryAccess();
    const QString oldRanges = s.mcpMemoryRanges();
    const QScopeGuard restore([&] {
        s.mcpMemoryAccess.setValue(oldAccess);
        s.mcpMemoryRanges.setValue(oldRanges);
    });
    s.mcpMemoryAccess.setValue(access);
    s.mcpMemoryRanges.setValue(ranges);

    DebuggerRunParameters rp;
    rp.setStartMode(DebuggerStartMode(startMode));
    rp.setToolChainAbi(Abi(Abi::ArmArchitecture, Abi::OS(os)));

    const Result<QString> result = mcpMemoryAccessGrant(rp, address, length);
    if (grant.isEmpty()) {
        QVERIFY2(!result, qPrintable(result ? *result : QString()));
        QVERIFY(!result.error().isEmpty());
    } else {
        QVERIFY2(result, qPrintable(result ? QString() : result.error()));
        QCOMPARE(*result, grant);
    }
}

// A buffer whose bytes say which offset they are at.
static const char s_memorySource[] = R"CPP(
int main()
{
    unsigned char pattern[64];
    for (int i = 0; i < 64; ++i)
        pattern[i] = (unsigned char)i;
    int first = pattern[0]; // MARKER: read
    return first;
}
)CPP";

void DebuggerUnitTests::testMcpReadsMemory_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpReadsMemory()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    CommonSettings &s = commonSettings();
    const int oldAccess = s.mcpMemoryAccess();
    const int oldLimit = s.mcpMemoryReadLimit();
    const QScopeGuard restore([&] {
        s.mcpMemoryAccess.setValue(oldAccess);
        s.mcpMemoryReadLimit.setValue(oldLimit);
    });
    s.mcpMemoryAccess.setValue(CommonSettings::McpMemoryOfProcesses);
    s.mcpMemoryReadLimit.setValue(4096);

    SteppingSession session(s_memorySource);
    const QString problem = session.start("MARKER: read");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> locals = callDebuggerTool("debugger_get_variables");
    QVERIFY2(locals, qPrintable(errorOf(locals)));
    const QString patternAddress = variableNamed(*locals, "pattern").value("address").toString();
    QVERIFY2(patternAddress.startsWith("0x"),
             QJsonDocument(*locals).toJson(QJsonDocument::Compact).constData());

    const Result<QJsonObject> read = callDebuggerTool(
        "debugger_read_memory", {{"address", patternAddress}, {"length", 16}});
    QVERIFY2(read, qPrintable(errorOf(read)));
    QCOMPARE(read->value("access").toString(), QString("process_memory"));
    QCOMPARE(read->value("bytes_read").toInt(), 16);
    QVERIFY(!read->value("truncated").toBool());
    const QJsonArray segments = read->value("segments").toArray();
    QCOMPARE(segments.size(), 1);
    QVERIFY(segments.first().toObject().value("readable").toBool());
    QCOMPARE(segments.first().toObject().value("data").toString(),
             QString("000102030405060708090a0b0c0d0e0f"));

    // Address zero is mapped in no process.
    const Result<QJsonObject> unmapped
        = callDebuggerTool("debugger_read_memory", {{"address", 0}, {"length", 16}});
    QVERIFY2(unmapped, qPrintable(errorOf(unmapped)));
    QCOMPARE(unmapped->value("bytes_read").toInt(), 0);
    int unreadable = 0;
    for (const QJsonValue &v : unmapped->value("segments").toArray()) {
        const QJsonObject segment = v.toObject();
        QVERIFY(!segment.value("readable").toBool());
        QVERIFY(!segment.contains("data"));
        QVERIFY(!segment.value("reason").toString().isEmpty());
        unreadable += segment.value("length").toInt();
    }
    QCOMPARE(unreadable, 16);

    // A whole page that cannot be read is one range, not one per byte.
    const Result<QJsonObject> unmappedPage
        = callDebuggerTool("debugger_read_memory", {{"address", 0}, {"length", 4096}});
    QVERIFY2(unmappedPage, qPrintable(errorOf(unmappedPage)));
    QCOMPARE(unmappedPage->value("bytes_read").toInt(), 0);
    QCOMPARE(unmappedPage->value("segments").toArray().size(), 1);

    // GDB does not say which address failed, so what shows the cost of
    // retrying is how many reads it is sent. Another read at an address the
    // retries never touch marks where those of the page end in the log.
    if (session.engine()->debuggerName().startsWith("GDB")) {
        const Result<QJsonObject> marker
            = callDebuggerTool("debugger_read_memory", {{"address", 0x2000}, {"length", 8}});
        QVERIFY2(marker, qPrintable(errorOf(marker)));
        const QString first = "-data-read-memory 0x0 x 1 1 4096";
        const QString last = "-data-read-memory 0x2000 x 1 1 8";
        QTRY_VERIFY(session.engine()->logContents().contains(last));
        const QString log = session.engine()->logContents();
        const qsizetype start = log.indexOf(first);
        QVERIFY(start >= 0);
        const QString between = log.mid(start + first.size(), log.indexOf(last) - start);
        QVERIFY2(between.count("-data-read-memory") <= 2,
                 qPrintable(QString("%1 more reads for one unreadable page")
                                .arg(between.count("-data-read-memory"))));
    }

    s.mcpMemoryReadLimit.setValue(8);
    const Result<QJsonObject> limited = callDebuggerTool(
        "debugger_read_memory", {{"address", patternAddress}, {"length", 16},
                                 {"encoding", "base64"}});
    QVERIFY2(limited, qPrintable(errorOf(limited)));
    QVERIFY(limited->value("truncated").toBool());
    QCOMPARE(limited->value("length").toInt(), 8);
    QCOMPARE(limited->value("requested_length").toInt(), 16);
    QCOMPARE(limited->value("next_address").toString().toULongLong(nullptr, 16),
             patternAddress.toULongLong(nullptr, 16) + 8);
    QCOMPARE(QByteArray::fromBase64(limited->value("segments").toArray().first().toObject()
                                        .value("data").toString().toLatin1()),
             QByteArray::fromHex("0001020304050607"));

    s.mcpMemoryAccess.setValue(CommonSettings::McpMemoryNever);
    const Result<QJsonObject> refused = callDebuggerTool(
        "debugger_read_memory", {{"address", patternAddress}, {"length", 4}});
    QVERIFY(!refused);
}

// The last 8 bytes of a page, followed by one that is not mapped.
static const char s_pageEdgeSource[] = R"CPP(
#include <sys/mman.h>
#include <unistd.h>

int main()
{
    const long page = sysconf(_SC_PAGESIZE);
    unsigned char *pages = (unsigned char *)mmap(nullptr, 2 * page, PROT_READ | PROT_WRITE,
                                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    for (long i = 0; i < page; ++i)
        pages[i] = (unsigned char)i;
    munmap(pages + page, page);
    unsigned long long edge = (unsigned long long)(pages + page - 8);
    int first = pages[0]; // MARKER: edge
    return first + int(edge & 0);
}
)CPP";

void DebuggerUnitTests::testMcpReadsMemoryUpToAnUnmappedPage_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpReadsMemoryUpToAnUnmappedPage()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    Toolchain *toolchain = nullptr;
    if (const QString reason = preCheckStepTests(nullptr, &toolchain); !reason.isEmpty())
        QSKIP(qPrintable(reason));
    if (toolchain->typeId() == ProjectExplorer::Constants::MSVC_TOOLCHAIN_TYPEID)
        QSKIP("The program maps its pages with mmap, which MSVC has none of.");

    CommonSettings &s = commonSettings();
    const int oldAccess = s.mcpMemoryAccess();
    const QScopeGuard restore([&] { s.mcpMemoryAccess.setValue(oldAccess); });
    s.mcpMemoryAccess.setValue(CommonSettings::McpMemoryOfProcesses);

    SteppingSession session(s_pageEdgeSource);
    const QString problem = session.start("MARKER: edge");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> locals = callDebuggerTool("debugger_get_variables");
    QVERIFY2(locals, qPrintable(errorOf(locals)));
    bool ok = false;
    const quint64 edge = variableNamed(*locals, "edge").value("value").toString()
                             .toULongLong(&ok, 0);
    QVERIFY2(ok && edge != 0, QJsonDocument(*locals).toJson(QJsonDocument::Compact).constData());

    const Result<QJsonObject> read = callDebuggerTool(
        "debugger_read_memory", {{"address", QString("0x%1").arg(edge, 0, 16)}, {"length", 16}});
    QVERIFY2(read, qPrintable(errorOf(read)));
    const QByteArray json = QJsonDocument(*read).toJson(QJsonDocument::Compact);
    QCOMPARE(read->value("bytes_read").toInt(), 8);
    const QJsonArray segments = read->value("segments").toArray();
    QVERIFY2(segments.size() >= 2, json.constData());
    const QJsonObject readable = segments.first().toObject();
    QVERIFY2(readable.value("readable").toBool(), json.constData());
    QCOMPARE(readable.value("length").toInt(), 8);
    QCOMPARE(readable.value("data").toString(), QString("f8f9fafbfcfdfeff"));
    int unreadable = 0;
    for (qsizetype i = 1; i < segments.size(); ++i) {
        const QJsonObject segment = segments.at(i).toObject();
        QVERIFY2(!segment.value("readable").toBool(), json.constData());
        unreadable += segment.value("length").toInt();
    }
    QCOMPARE(unreadable, 8);
}

void DebuggerUnitTests::testMcpEvaluatesAnExpressionOfTheCurrentStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpEvaluatesAnExpressionOfTheCurrentStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: after-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const auto evaluate = [](const QString &expression) {
        return callDebuggerTool("debugger_evaluate_expression", {{"expression", expression}});
    };

    const Result<QJsonObject> second = evaluate("second");
    QVERIFY2(second, qPrintable(errorOf(second)));
    QCOMPARE(second->value("value").toString(), QString("2"));
    QCOMPARE(second->value("type").toString(), QString("int"));
    QVERIFY(stopIdOf(*second) > 0);

    const Result<QJsonObject> sum = evaluate("first + second");
    QVERIFY2(sum, qPrintable(errorOf(sum)));
    QCOMPARE(sum->value("value").toString(), QString("3"));

    // An expression the user watches already is answered, and stays watched.
    const Result<QJsonObject> watched
        = callDebuggerTool("debugger_add_watch_expression", {{"expression", "first"}});
    QVERIFY2(watched, qPrintable(errorOf(watched)));
    const QString watchIname = watched->value("iname").toString();
    const QScopeGuard unwatch([watchIname] {
        callDebuggerTool("debugger_remove_watch_expression", {{"iname", watchIname}});
    });
    const Result<QJsonObject> first = evaluate("first");
    QVERIFY2(first, qPrintable(errorOf(first)));
    QCOMPARE(first->value("value").toString(), QString("1"));
    const Result<QJsonObject> variables
        = callDebuggerTool("debugger_get_variables", {{"include_watchers", true}});
    QVERIFY2(variables, qPrintable(errorOf(variables)));
    bool stillWatched = false;
    for (const QJsonValue &v : variables->value("variables").toArray())
        stillWatched |= v.toObject().value("iname").toString() == watchIname;
    QVERIFY(stillWatched);
}

void DebuggerUnitTests::testMcpEvaluationKeepsTheUsersWatch_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpEvaluationKeepsTheUsersWatch()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    const QString expression = "second";
    QVERIFY2(!WatchHandler::isWatched(expression),
             "a watch of \"second\" is left from an earlier run; remove it in the session");
    {
        SteppingSession session;
        const QString problem = session.start("MARKER: after-call");
        QVERIFY2(problem.isEmpty(), qPrintable(problem));

        // Evaluating adds and removes a watcher of its own.
        const Result<QJsonObject> evaluated
            = callDebuggerTool("debugger_evaluate_expression", {{"expression", expression}});
        QVERIFY2(evaluated, qPrintable(errorOf(evaluated)));

        const Result<QJsonObject> watched
            = callDebuggerTool("debugger_add_watch_expression", {{"expression", expression}});
        QVERIFY2(watched, qPrintable(errorOf(watched)));
    }

    // The session has ended; the user's watch outlives it.
    QVERIFY(WatchHandler::watchedExpressions().contains(expression));

    // It is there in the next session, which is also where it can be removed.
    SteppingSession next;
    const QString problem = next.start("MARKER: after-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
    const Result<QJsonObject> variables
        = callDebuggerTool("debugger_get_variables", {{"include_watchers", true}});
    QVERIFY2(variables, qPrintable(errorOf(variables)));
    QString iname;
    for (const QJsonValue &v : variables->value("variables").toArray()) {
        const QJsonObject variable = v.toObject();
        if (variable.value("iname").toString().startsWith("watch.")
            && variable.value("name").toString() == expression) {
            iname = variable.value("iname").toString();
        }
    }
    QVERIFY2(!iname.isEmpty(), "the watch is not there in the next session");
    const Result<QJsonObject> removed
        = callDebuggerTool("debugger_remove_watch_expression", {{"iname", iname}});
    QVERIFY2(removed, qPrintable(errorOf(removed)));
    QVERIFY(!WatchHandler::isWatched(expression));
}

void DebuggerUnitTests::testMcpEvaluationsShareATemporaryWatch_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpEvaluationsShareATemporaryWatch()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    const QString problem = session.start("MARKER: after-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    // Two evaluations of the same expression at once wait on one watcher, and
    // the first to answer must not take it from the other.
    const QString shared = "second + 1";
    QVERIFY(!WatchHandler::isWatched(shared));
    std::optional<Result<QJsonObject>> first;
    std::optional<Result<QJsonObject>> other;
    callMcpToolForTests("debugger_evaluate_expression", {{"expression", shared}},
                        [&first](Result<QJsonObject> result) { first = result; });
    callMcpToolForTests("debugger_evaluate_expression", {{"expression", shared}},
                        [&other](Result<QJsonObject> result) { other = result; });
    QVERIFY(QTest::qWaitFor([&] { return first && other; }, 60000));
    QVERIFY2(*first, qPrintable(errorOf(*first)));
    QVERIFY2(*other, qPrintable(errorOf(*other)));
    QCOMPARE((*other)->value("value").toString(), (*first)->value("value").toString());
    QVERIFY(!WatchHandler::isWatched(shared));

    // A watch the user adds while an evaluation waits is the user's.
    const QString kept = "second + 2";
    QVERIFY(!WatchHandler::isWatched(kept));
    std::optional<Result<QJsonObject>> evaluated;
    std::optional<Result<QJsonObject>> watched;
    callMcpToolForTests("debugger_evaluate_expression", {{"expression", kept}},
                        [&evaluated](Result<QJsonObject> result) { evaluated = result; });
    callMcpToolForTests("debugger_add_watch_expression", {{"expression", kept}},
                        [&watched](Result<QJsonObject> result) { watched = result; });
    QVERIFY(QTest::qWaitFor([&] { return evaluated && watched; }, 60000));
    QVERIFY2(*evaluated, qPrintable(errorOf(*evaluated)));
    QVERIFY2(*watched, qPrintable(errorOf(*watched)));
    QVERIFY(WatchHandler::isWatched(kept));
    const QString iname = session.engine()->watchHandler()->watcherName(kept);
    QVERIFY(session.engine()->watchHandler()->findItem(iname));
    const Result<QJsonObject> removed
        = callDebuggerTool("debugger_remove_watch_expression", {{"iname", iname}});
    QVERIFY2(removed, qPrintable(errorOf(removed)));
    QVERIFY(!WatchHandler::isWatched(kept));
}

static QJsonObject capabilityNamed(const QJsonObject &answer, const QString &name)
{
    for (const QJsonValue &c : answer.value("capabilities").toArray()) {
        if (c.toObject().value("name").toString() == name)
            return c.toObject();
    }
    return {};
}

void DebuggerUnitTests::testMcpReportsCapabilities_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testMcpReportsCapabilities()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    CommonSettings &s = commonSettings();
    const int oldAccess = s.mcpMemoryAccess();
    const QScopeGuard restore([&] { s.mcpMemoryAccess.setValue(oldAccess); });
    s.mcpMemoryAccess.setValue(CommonSettings::McpMemoryOfProcesses);

    SteppingSession session;
    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));

    const Result<QJsonObject> paused = callDebuggerTool("debugger_get_capabilities");
    QVERIFY2(paused, qPrintable(errorOf(paused)));
    QVERIFY(paused->value("has_session").toBool());

    const QJsonObject env = paused->value("environment").toObject();
    QVERIFY2(env.value("debugger").toString().contains("LLDB", Qt::CaseInsensitive)
                 || !HostOsInfo::isMacHost(),
             qPrintable(env.value("debugger").toString()));
    QCOMPARE(env.value("target_architecture").toString(),
             Abi::toString(Abi::hostAbi().architecture()));
    QCOMPARE(env.value("target_os").toString(), Abi::toString(Abi::hostAbi().os()));
    QCOMPARE(env.value("connection_type").toString(), QString("local"));
    QCOMPARE(env.value("session_type").toString(), QString("launch"));

    const QStringList availableWhilePaused{"execution_control", "call_stack", "variables",
                                           "registers", "memory_read", "threads"};
    for (const QString &name : availableWhilePaused) {
        const QJsonObject c = capabilityNamed(*paused, name);
        QCOMPARE(c.value("status").toString() + ' ' + name, "available " + name);
        QVERIFY(c.value("exposed").toBool());
    }
    QCOMPARE(capabilityNamed(*paused, "memory_read").value("permission").toString(),
             QString("granted"));
    QCOMPARE(capabilityNamed(*paused, "reverse_stepping").value("status").toString(),
             QString("not_exposed"));

    s.mcpMemoryAccess.setValue(CommonSettings::McpMemoryNever);
    const Result<QJsonObject> denied = callDebuggerTool("debugger_get_capabilities");
    QVERIFY2(denied, qPrintable(errorOf(denied)));
    const QJsonObject memory = capabilityNamed(*denied, "memory_read");
    QCOMPARE(memory.value("status").toString(), QString("permission_denied"));
    QCOMPARE(memory.value("backend_support").toString(), QString("supported"));
    QVERIFY(!memory.value("reason").toString().isEmpty());

    // Once the program has run to its end, nothing that needs a stop is available.
    const Result<QJsonObject> ran
        = callDebuggerTool("debugger_continue", {{"wait_for_completion", true}});
    QVERIFY2(ran, qPrintable(errorOf(ran)));
    QCOMPARE(ran->value("status").toString(), QString("failed"));
    const Result<QJsonObject> ended = callDebuggerTool("debugger_get_capabilities");
    QVERIFY2(ended, qPrintable(errorOf(ended)));
    QCOMPARE(capabilityNamed(*ended, "call_stack").value("status").toString(),
             QString("invalid_state"));
}

void DebuggerUnitTests::testDisabledBreakpointDoesNotStop_data()
{
    addBackendRows();
}

void DebuggerUnitTests::testDisabledBreakpointDoesNotStop()
{
    QFETCH(bool, generic);
    const BackendUnderTest backend(generic);
    if (const QString reason = backend.reasonItIsNotUnderTest(); !reason.isEmpty())
        QSKIP(qPrintable(reason));

    SteppingSession session;
    // A disabled breakpoint on a line that runs before the marker: if it
    // stops the program, the session never gets to the marker.
    BreakpointParameters params;
    params.type = BreakpointByFileAndLine;
    params.fileName = session.sourceFile();
    params.textPosition = {session.markerLine("MARKER: at-call") - 1, -1};
    params.enabled = false;
    const GlobalBreakpoint disabled = BreakpointManager::createBreakpoint(params);
    QVERIFY(disabled);
    session.keepBreakpoint(disabled);
    // cdb takes a breakpoint on a file of the same name in another directory
    // for one on this file, and the two end up on the same address. Disabling
    // that one must not disable the session's own.
    QTemporaryDir otherDir;
    QVERIFY(otherDir.isValid());
    params.fileName = FilePath::fromString(otherDir.path()) / "stepping.cpp";
    params.textPosition = {session.markerLine("MARKER: at-call"), -1};
    const GlobalBreakpoint elsewhere = BreakpointManager::createBreakpoint(params);
    QVERIFY(elsewhere);
    session.keepBreakpoint(elsewhere);
    const QScopeGuard remove([disabled, elsewhere] {
        if (disabled)
            disabled->deleteBreakpoint();
        if (elsewhere)
            elsewhere->deleteBreakpoint();
    });

    const QString problem = session.start("MARKER: at-call");
    QVERIFY2(problem.isEmpty(), qPrintable(problem));
}

static QStringList s_capturedMessages;

static void captureMessages(QtMsgType, const QMessageLogContext &, const QString &message)
{
    s_capturedMessages.append(message);
}

// The address a disassembly was asked for is not always in the answer: the
// location can have moved on, and a block does not cover every address it was
// reached from. There is no line to mark then, and line zero is not one.
void DebuggerUnitTests::testDisassemblyThatMissesTheAddressMarksNoLine()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] {
        delete engine;
        EditorManager::closeAllEditors(false);
    });
    engine->setRunParameters({});

    Location location(quint64(0x1000));
    location.setNeedsMarker(true);

    DisassemblerLines lines;
    DisassemblerLine line;
    line.address = 0x2000;
    line.data = "nop";
    lines.appendLine(line);

    s_capturedMessages.clear();
    QtMessageHandler previous = qInstallMessageHandler(captureMessages);
    const QScopeGuard restoreHandler([previous] { qInstallMessageHandler(previous); });

    DisassemblerAgent agent(engine);
    agent.setLocation(location);
    agent.setContents(lines);

    const QString complaint = Utils::findOrDefault(s_capturedMessages, [](const QString &m) {
        return m.contains("SOFT ASSERT");
    });
    QVERIFY2(complaint.isEmpty(), qPrintable("marking a line the disassembly has "
                                             "not got - " + complaint));
}

// Operating by instruction sends every location to the disassembler, so the
// question of whether one has machine code behind it is the only thing
// standing between a QML frame and a disassembly of nothing.
void DebuggerUnitTests::testOnlyMachineCodeIsOfferedADisassembly()
{
    QVERIFY2(!Location().canBeDisassembled(),
             "a location naming neither an address nor a function offered one");

    StackFrame native;
    native.function = "main";
    native.address = 0x1000;
    QVERIFY(Location(native).canBeDisassembled());

    StackFrame interpreted;
    interpreted.language = QmlLanguage;
    interpreted.function = "expression for onClicked";
    interpreted.file = FilePath::fromUserInput("Main.qml");
    interpreted.line = 26;
    QVERIFY2(!Location(interpreted).canBeDisassembled(),
             "a QML frame offered a disassembly of code it does not have");
}

// gdb selects only native frames, so a frame of QML compiled to C++ must not
// pass for an interpreter frame because of its file name.
void DebuggerUnitTests::testCompiledQmlFramesAreNative()
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi frameMi;
    frameMi.fromString(R"({level="0",function="onTriggered",file="Main.qml",line="30"})",
                       decoder);

    DebuggerRunParameters rp;
    QCOMPARE(StackFrame::parseFrame(frameMi, rp).language, QmlLanguage);

    rp.setQmlCompiledToNative(true);
    QCOMPARE(StackFrame::parseFrame(frameMi, rp).language, CppLanguage);
}

// The engine gdb kits get by default, not the GdbImpl the backend tests drive.
void DebuggerUnitTests::testGdbEngineTakesCompiledQmlBreakpoints()
{
    BreakpointParameters fileAndLine(BreakpointByFileAndLine);
    fileAndLine.fileName = FilePath::fromUserInput("Main.qml");
    fileAndLine.textPosition.line = 30;
    const BreakpointParameters signalEmit(BreakpointOnQmlSignalEmit);

    GdbEngine engine;
    const DebuggerEngine &asEngine = engine;
    DebuggerRunParameters rp;
    engine.setRunParameters(rp);
    QVERIFY(!asEngine.acceptsBreakpoint(fileAndLine));

    rp.setQmlCompiledToNative(true);
    engine.setRunParameters(rp);
    QVERIFY(asEngine.acceptsBreakpoint(fileAndLine));
    QVERIFY2(!asEngine.acceptsBreakpoint(signalEmit),
             "gdb took a breakpoint only the QML engine can handle");
}

void DebuggerUnitTests::testABreakpointDropsThePlacesItNoLongerStandsIn()
{
    BreakpointItem bp({});
    for (const QString &responseId : QStringList{"3.1", "3.2", "3.3"}) {
        auto sub = new SubBreakpointItem;
        sub->responseId = responseId;
        bp.appendChild(sub);
    }
    QCOMPARE(bp.childCount(), 3);

    bp.keepOnlySubBreakpoints({"3.1", "3.3"});
    QCOMPARE(bp.childCount(), 2);
    QCOMPARE(bp.childAt(0)->responseId, QString("3.1"));
    QCOMPARE(bp.childAt(1)->responseId, QString("3.3"));

    bp.keepOnlySubBreakpoints({});
    QCOMPARE(bp.childCount(), 0);

    auto engine = new GenericDebuggerEngine("test", new RecordingBackend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    const auto constMi = [](const QString &name, const QString &data) {
        GdbMi child;
        child.m_type = GdbMi::Const;
        child.m_name = name;
        child.m_data = data;
        return child;
    };
    GdbMi locations;
    locations.m_type = GdbMi::List;
    locations.m_name = "locations";
    for (const QString &responseId : QStringList{"3.1", "3.2", "3.3"}) {
        GdbMi location;
        location.m_type = GdbMi::Tuple;
        location.addChild(constMi("number", responseId));
        locations.addChild(location);
    }
    GdbMi several;
    several.m_type = GdbMi::Tuple;
    several.addChild(constMi("number", "3"));
    several.addChild(locations);

    BreakpointItem reported({});
    engine->applyBkptData(several, Breakpoint(&reported));
    QCOMPARE(reported.childCount(), 3);

    GdbMi single;
    single.m_type = GdbMi::Tuple;
    single.addChild(constMi("number", "4"));
    engine->applyBkptData(single, Breakpoint(&reported));
    QVERIFY2(reported.childCount() == 0,
             "the rows of the places the breakpoint stood in before are still there, and each "
             "of them offers to switch off a place it no longer has");
}

void DebuggerUnitTests::testClaimedQmlBreakpointsGetNoQmlHint()
{
    BreakpointParameters fileAndLine(BreakpointByFileAndLine);
    fileAndLine.fileName = FilePath::fromUserInput("Main.qml");
    fileAndLine.textPosition.line = 30;
    const GlobalBreakpoint qml = BreakpointManager::createBreakpoint(fileAndLine);
    const GlobalBreakpoint signalEmit = BreakpointManager::createBreakpoint(
        BreakpointParameters(BreakpointOnQmlSignalEmit));
    const QScopeGuard remove([qml, signalEmit] {
        if (qml)
            qml->deleteBreakpoint();
        if (signalEmit)
            signalEmit->deleteBreakpoint();
    });
    QVERIFY(qml);
    QVERIFY(signalEmit);

    GdbEngine engine;
    DebuggerRunParameters rp;
    rp.setQmlCompiledToNative(true);
    engine.setRunParameters(rp);

    const Result<> res = checkBreakpoints({&engine});
    QVERIFY2(!res, "the signal emit breakpoint was not reported as unhandled");
    QVERIFY2(!res.error().contains("cannot stop in it"), qPrintable(res.error()));
    QVERIFY2(!res.error().contains("QML debugging needs"), qPrintable(res.error()));
}

void DebuggerUnitTests::testUnhandledQmlBreakpointsGetAHint()
{
    BreakpointParameters fileAndLine(BreakpointByFileAndLine);
    fileAndLine.fileName = FilePath::fromUserInput("Main.qml");
    fileAndLine.textPosition.line = 30;
    const GlobalBreakpoint qml = BreakpointManager::createBreakpoint(fileAndLine);
    const QScopeGuard remove([qml] {
        if (qml)
            qml->deleteBreakpoint();
    });
    QVERIFY(qml);

    // A core file engine accepts no breakpoint at all.
    GdbEngine engine;
    DebuggerRunParameters rp;
    rp.setStartMode(AttachToCore);
    rp.setQmlCompiledToNative(true);
    engine.setRunParameters(rp);

    Result<> res = checkBreakpoints({&engine});
    QVERIFY2(!res, "the QML breakpoint was not reported as unhandled");
    QVERIFY2(res.error().contains("cannot stop in it"), qPrintable(res.error()));
    QVERIFY2(!res.error().contains("QML debugging needs"), qPrintable(res.error()));

    rp.setQmlCompiledToNative(false);
    engine.setRunParameters(rp);
    res = checkBreakpoints({&engine});
    QVERIFY2(!res, "the QML breakpoint was not reported as unhandled");
    QVERIFY2(res.error().contains("QML debugging needs"), qPrintable(res.error()));
    QVERIFY2(!res.error().contains("cannot stop in it"), qPrintable(res.error()));
}

void DebuggerUnitTests::testKitWithCompiledQmlDisablesQmlDebugging()
{
    Kit *kit = KitManager::defaultKit();
    QVERIFY(kit);

    auto &check = qmlCompiledToNativeCheck();
    const auto saved = check;
    const QScopeGuard restore([&check, saved] { check = saved; });
    check = [kit](const Kit *k) { return k == kit; };

    const std::unique_ptr<RunControl> runControl(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    runControl->setKit(kit);
    AspectContainerData data;
    data.append(DebuggerRunConfigurationAspect::Data::createCppAndQmlTestData());
    runControl->setAspectDataForTest(data);

    const DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl.get());
    QVERIFY(rp.isQmlCompiledToNative());
    QVERIFY(!rp.isQmlDebugging());

    // Only QML was asked for, so the kit's debugger has to take over what it can.
    AspectContainerData qmlOnly;
    qmlOnly.append(DebuggerRunConfigurationAspect::Data::createQmlTestData());
    runControl->setAspectDataForTest(qmlOnly);
    const DebuggerRunParameters native = DebuggerRunParameters::fromRunControl(runControl.get());
    QVERIFY(native.isQmlCompiledToNative());
    QVERIFY(!native.isQmlDebugging());
    if (DebuggerKitAspect::debugger(kit))
        QVERIFY(native.isCppDebugging());
    else
        QVERIFY(!native.m_validationErrors.isEmpty());

    // The kit's debugger is validated once when C++ debugging was requested already.
    Kit noDebugger;
    DebuggerKitAspect::setDebugger(&noDebugger, {});
    check = [&noDebugger](const Kit *k) { return k == &noDebugger; };
    const std::unique_ptr<RunControl> noDebuggerRun(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    noDebuggerRun->setKit(&noDebugger);
    noDebuggerRun->setAspectDataForTest(data);
    const DebuggerRunParameters once = DebuggerRunParameters::fromRunControl(noDebuggerRun.get());
    QCOMPARE(once.m_validationErrors.size(), 1);

    // Without a debugger in the kit a plugin can still bring a backend of its own.
    noDebuggerRun->setAspectDataForTest(qmlOnly);
    DebuggerRunParameters brought = DebuggerRunParameters::fromRunControl(noDebuggerRun.get());
    QVERIFY(!brought.isCppDebugging());
    QCOMPARE(brought.m_validationErrors.size(), 1);
    brought.setEngineInterface("Plugin", [](const DebuggerRunParameters &) {
        return std::unique_ptr<DebuggerEngineInterface>();
    });
    QVERIFY(brought.isCppDebugging());
    QVERIFY(brought.m_validationErrors.isEmpty());

    check = {};
    runControl->setAspectDataForTest(data);
    const DebuggerRunParameters plain = DebuggerRunParameters::fromRunControl(runControl.get());
    QVERIFY(plain.isQmlDebugging());
}

void DebuggerUnitTests::testAnEmptyDisassemblyLeavesTheViewAlone()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] {
        delete engine;
        EditorManager::closeAllEditors(false);
    });
    engine->setRunParameters({});

    Location location(quint64(0x1000));
    location.setNeedsMarker(true);

    DisassemblerAgent agent(engine);
    agent.setLocation(location);

    const int before = DocumentModel::entryCount();
    agent.setContents({});
    QCOMPARE(DocumentModel::entryCount(), before);
}

static QList<int> linkedLines(const DisassemblerAgent &agent, IEditor *editor)
{
    QList<int> lines;
    const auto widget = TextEditor::TextEditorWidget::fromEditor(editor);
    for (const QTextEdit::ExtraSelection &selection :
         widget->extraSelections(agent.linkedLinesSelection())) {
        lines.append(selection.cursor.document()->findBlock(selection.cursor.selectionStart())
                         .blockNumber());
    }
    return lines;
}

void DebuggerUnitTests::testDisassemblyLinksToItsSource()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath sourceFile = FilePath::fromString(tmp.path()) / "main.cpp";
    QVERIFY(sourceFile.writeFileContents("int main()\n{\n    int x = 1;\n    return x;\n}\n"));

    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] {
        delete engine;
        EditorManager::closeAllEditors(false);
    });
    engine->setRunParameters({});

    BoolAspect &besideSource = settings().showSourceBesideDisassembly;
    const bool wasBesideSource = besideSource();
    besideSource.setValue(true);
    const QScopeGuard restoreSetting([&besideSource, wasBesideSource] {
        besideSource.setValue(wasBesideSource);
    });

    IEditor *source = EditorManager::openEditor(sourceFile);
    QVERIFY(source);

    StackFrame frame;
    frame.function = "main";
    frame.address = 0x1000;
    frame.file = sourceFile;
    frame.line = 3;
    frame.usable = true;

    DisassemblerLines lines;
    DisassemblerLine line;
    line.lineNumber = 3;
    line.data = "    int x = 1;";
    lines.appendLine(line);
    line = {};
    line.address = 0x1000;
    line.data = "movl   $0x1,-0x4(%rbp)";
    lines.appendLine(line);
    line = {};
    line.lineNumber = 4;
    line.data = "    return x;";
    lines.appendLine(line);
    line = {};
    line.address = 0x1007;
    line.data = "mov    -0x4(%rbp),%eax";
    lines.appendLine(line);
    line = {};
    line.address = 0x100a;
    line.data = "pop    %rbp";
    lines.appendLine(line);

    DisassemblerAgent agent(engine);
    agent.setLocation(Location(frame));
    agent.setContents(lines);

    IEditor *disassembly = EditorManager::currentEditor();
    QVERIFY(disassembly && disassembly != source);
    QVERIFY2(EditorManager::visibleEditors().contains(source),
             "the disassembly took the place of the source");
    QVERIFY(EditorManager::viewIdForEditor(disassembly) != EditorManager::viewIdForEditor(source));
    QVERIFY2(DocumentModel::editorsForDocument(source->document()).size() == 1,
             "the split made for the disassembly holds a copy of the source");

    // The cursor is on the instruction at the address, which is from line 3.
    QCOMPARE(linkedLines(agent, source), QList<int>{2});
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{0, 1}));

    EditorManager::activateEditor(source);
    TextEditor::TextEditorWidget::fromEditor(source)->gotoLine(4);
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{2, 3, 4}));
    QCOMPARE(linkedLines(agent, source), QList<int>{3});

    TextEditor::TextEditorWidget::fromEditor(source)->gotoLine(1);
    QVERIFY2(linkedLines(agent, disassembly).isEmpty(), "a line without instructions linked some");
    QVERIFY(linkedLines(agent, source).isEmpty());

    // A step within the function brings the same disassembly again.
    const QTextDocument *disassemblyText
        = TextEditor::TextEditorWidget::fromEditor(disassembly)->document();
    const int revision = disassemblyText->revision();
    agent.setContents(lines);
    QCOMPARE(disassemblyText->revision(), revision);
    QCOMPARE(EditorManager::currentEditor(), disassembly);
    QCOMPARE(linkedLines(agent, source), QList<int>{2});
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{0, 1}));

    // The line under the mouse takes over from the one the cursor is on, on
    // either side, until the mouse leaves.
    const auto hover = [](IEditor *editor, int line) {
        const auto widget = TextEditor::TextEditorWidget::fromEditor(editor);
        const QTextBlock block = widget->document()->findBlockByNumber(line);
        const QPoint pos = widget->cursorRect(QTextCursor(block)).center();
        QMouseEvent move(QEvent::MouseMove, pos, widget->viewport()->mapToGlobal(pos),
                         Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget->viewport(), &move);
    };
    const auto leave = [](IEditor *editor) {
        QEvent event(QEvent::Leave);
        QCoreApplication::sendEvent(
            TextEditor::TextEditorWidget::fromEditor(editor)->viewport(), &event);
    };
    hover(disassembly, 3);
    QCOMPARE(linkedLines(agent, source), QList<int>{3});
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{2, 3, 4}));
    leave(disassembly);
    QCOMPARE(linkedLines(agent, source), QList<int>{2});

    hover(source, 3);
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{2, 3, 4}));
    hover(source, 0);
    QVERIFY2(linkedLines(agent, disassembly).isEmpty(), "a line without instructions linked some");
    leave(source);
    QCOMPARE(linkedLines(agent, disassembly), (QList<int>{0, 1}));

    // Another session showing the same source leaves these links alone.
    auto otherEngine = new GenericDebuggerEngine("other", new RecordingBackend);
    const QScopeGuard otherCleanup([otherEngine] { delete otherEngine; });
    otherEngine->setRunParameters({});
    DisassemblerAgent otherAgent(otherEngine);
    otherAgent.setLocation(Location(frame));
    otherAgent.setContents(lines);
    QCOMPARE(linkedLines(otherAgent, source), QList<int>{2});
    EditorManager::activateEditor(disassembly);
    QVERIFY(linkedLines(otherAgent, source).isEmpty());
    QCOMPARE(linkedLines(agent, source), QList<int>{2});

    // A source editor opened again gets the links it shows.
    QVERIFY(EditorManager::closeDocuments({source->document()}, false));
    QVERIFY(!DocumentModel::documentForFilePath(sourceFile));
    source = EditorManager::openEditor(sourceFile, {}, EditorManager::DoNotChangeCurrentEditor);
    QVERIFY(source);
    QCOMPARE(EditorManager::currentEditor(), disassembly);
    QCOMPARE(linkedLines(agent, source), QList<int>{2});

    // With the source's split gone, the split made for it again holds no
    // copy of the disassembly.
    Command *removeAllSplits = ActionManager::command(Core::Constants::REMOVE_ALL_SPLITS);
    QVERIFY(removeAllSplits && removeAllSplits->action()->isEnabled());
    removeAllSplits->action()->trigger();
    QVERIFY(!EditorManager::hasSplitter());
    EditorManager::activateEditor(disassembly);
    QCOMPARE(EditorManager::visibleEditors(), QList<IEditor *>{disassembly});
    const int sourceViewId = agent.sourceViewId(sourceFile);
    QVERIFY(sourceViewId != 0);
    QVERIFY(sourceViewId != EditorManager::viewIdForEditor(disassembly));
    QVERIFY2(DocumentModel::editorsForDocument(disassembly->document()).size() == 1,
             "the split made for the source holds a copy of the disassembly");
    removeAllSplits->action()->trigger();

    // The file named by the debug information goes through the source path mapping.
    auto mappedEngine = new GenericDebuggerEngine("mapped", new RecordingBackend);
    const QScopeGuard mappedCleanup([mappedEngine] { delete mappedEngine; });
    DebuggerRunParameters rp;
    rp.insertSourcePath("/home/qt/work", tmp.path());
    mappedEngine->setRunParameters(rp);
    DisassemblerLines mappedLines;
    for (int i = 0; i < lines.size(); ++i) {
        DisassemblerLine mappedLine = lines.at(i);
        if (mappedLine.isCode())
            mappedLine.fileName = "/home/qt/work/main.cpp";
        mappedLines.appendLine(mappedLine);
    }
    DisassemblerAgent mappedAgent(mappedEngine);
    mappedAgent.setLocation(Location(frame));
    mappedAgent.setContents(mappedLines);
    QCOMPARE(linkedLines(mappedAgent, source), QList<int>{2});
}

void DebuggerUnitTests::testMissingSourceMessage()
{
    const FilePath file = FilePath::fromUserInput("/src/foo.cpp");
    QVERIFY(msgMissingSource(file, "main").contains(file.toUserOutput()));
    QVERIFY(msgMissingSource({}, "main").contains("main"));

    // A location with neither must not be reported as an empty name.
    QVERIFY(!msgMissingSource({}, {}).contains("\"\""));
}

// A stack the way a backend reports one, with usability stated rather than
// looked up on disk.
static GdbMi stackReply(const QStringList &frames)
{
    QStringDecoder decoder(QStringDecoder::Utf8);
    GdbMi data;
    data.fromString('[' + frames.join(',') + ']', decoder);
    return data;
}

void DebuggerUnitTests::testASkippedTopFrameNamesItsMissingSource()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    const QStringList frames
        = {R"({level="0",function="abort",file="/usr/src/abort.c",usable="0"})",
           R"({level="1",function="main",file="/src/main.cpp",usable="1"})"};
    // As the message shows it: with backslashes on Windows.
    const QString skippedFile = FilePath::fromUserInput("/usr/src/abort.c").toUserOutput();
    const auto mentions = [engine, skippedFile] {
        return engine->logWindow()->inputContents().count(skippedFile);
    };

    engine->stackHandler()->setFramesAndCurrentIndex(stackReply(frames), true);
    QCOMPARE(engine->stackHandler()->currentIndex(), 1);
    QVERIFY2(mentions() > 0, qPrintable("nothing said about the frame that was skipped:\n"
                                        + engine->logWindow()->inputContents()));

    // The same stop refreshing its stack does not say it again.
    const int said = mentions();
    engine->stackHandler()->setFramesAndCurrentIndex(stackReply(frames), true);
    QCOMPARE(mentions(), said);
}

void DebuggerUnitTests::testSkippedMachineryFramesStaySilent()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    const QStringList frames
        = {R"({level="0",function="qt_message_fatal",machinery="1",usable="0"})",
           R"({level="1",function="main",file="/src/main.cpp",usable="1"})"};

    const bool wasCollapsing = settings().collapseMachineryFrames();
    const QScopeGuard restore([wasCollapsing] {
        settings().collapseMachineryFrames.setValue(wasCollapsing);
    });

    // Collapsed, the run is a placeholder for frames nobody asked to see;
    // expanded, it is the machinery frames themselves. Skipping either is
    // deliberate, so there is nothing to explain.
    for (const bool collapsing : {true, false}) {
        settings().collapseMachineryFrames.setValue(collapsing);
        const QString before = engine->logWindow()->inputContents();
        engine->stackHandler()->setFramesAndCurrentIndex(stackReply(frames), true);
        QVERIFY2(engine->logWindow()->inputContents() == before,
                 qPrintable(QString("a machinery frame was explained away (collapsing %1): %2")
                                .arg(collapsing)
                                       .arg(engine->logWindow()->inputContents()
                                                .mid(before.size()))));
    }
}

// What a session leaves behind when it ends without reporting an empty stack
// first. Whatever still asks for the current frame then must not be sent to
// one that is gone.
void DebuggerUnitTests::testAnEmptiedStackHasNoCurrentFrame()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});

    StackHandler *stack = engine->stackHandler();
    stack->setFramesAndCurrentIndex(
        stackReply({R"({level="0",function="main",file="/src/main.cpp",usable="1"})"}), true);
    QCOMPARE(stack->currentIndex(), 0);

    stack->removeAll();
    QCOMPARE(stack->currentIndex(), -1);

    s_capturedMessages.clear();
    QtMessageHandler previous = qInstallMessageHandler(captureMessages);
    const QScopeGuard restoreHandler([previous] { qInstallMessageHandler(previous); });
    QVERIFY(!stack->currentFrame().isUsable());
    const QString complaint = Utils::findOrDefault(s_capturedMessages, [](const QString &m) {
        return m.contains("SOFT ASSERT");
    });
    QVERIFY2(complaint.isEmpty(), qPrintable(complaint));
}

void DebuggerUnitTests::testAFailedRunRestoresOnlyAPendingReset()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] { delete engine; });
    engine->setRunParameters({});
    StackHandler *stack = engine->stackHandler();

    // The frames of the stop the run reached complete the reset.
    stack->scheduleResetLocation();
    stack->setFramesAndCurrentIndex(
        stackReply({R"({level="0",function="main",file="/src/main.cpp",usable="1"})"}), true);
    QVERIFY(stack->isContentsValid());

    // A run that did not reset anything has nothing to restore.
    stack->cancelResetLocation();
    QVERIFY(stack->isContentsValid());

    stack->scheduleResetLocation();
    QVERIFY(!stack->isContentsValid());
    stack->cancelResetLocation();
    QVERIFY(stack->isContentsValid());

    // Once the target ran, a later failed run does not bring back the stack
    // of the stop before.
    stack->scheduleResetLocation();
    stack->commitResetLocation();
    stack->cancelResetLocation();
    QVERIFY(!stack->isContentsValid());
}

// Attaching stops the inferior wherever it happens to be, and a backend whose
// run asks for it to go on resumes that stop by itself. Where every one of
// these tests starts: such a stop, reported as what it is.
static void reportAnAttachStopThatIsResumed(RecordingBackend *backend)
{
    emit backend->inferiorEvent(InferiorEvent::EngineSetupOk);
    emit backend->inferiorEvent(InferiorEvent::RunAndInferiorStopOkAndResuming);
}

// On Android that stop is every thread parked in libc's syscall(). With no
// source for the frame, reading it puts disassembly in the editor and asks for
// more of it, all before the run has reached main().
void DebuggerUnitTests::testAResumedAttachStopLeavesTheViewsAlone()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});

    engine->start();
    reportAnAttachStopThatIsResumed(backend);

    QCOMPARE(engine->state(), InferiorStopOk);
    QVERIFY2(!backend->refreshes.contains(RefreshKind::FullStack),
             "the stack was read from an attach stop the backend resumes at once");
    QVERIFY2(!backend->refreshes.contains(RefreshKind::Threads),
             "the threads were read from an attach stop the backend resumes at once");
}

// That stop is the only one an attach has, so a resume the debugger refuses
// leaves the session on it with nothing in the views, unless the engine goes
// back for the stop it skipped.
void DebuggerUnitTests::testARefusedResumeReadsTheAttachStopAfterAll()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});

    engine->start();
    reportAnAttachStopThatIsResumed(backend);

    emit backend->inferiorEvent(InferiorEvent::RunRequested);
    emit backend->inferiorEvent(InferiorEvent::RunFailed);

    QCOMPARE(engine->state(), InferiorStopOk);
    QVERIFY2(backend->refreshes.contains(RefreshKind::FullStack),
             "the resume was refused and the stack of the stop it skipped stayed unread");
    QVERIFY2(backend->refreshes.contains(RefreshKind::Threads),
             "the threads of that stop stayed unread as well");
}

// What is owed is that one stop. Once the inferior has run, the views belong
// to wherever it went, and a run that fails later must not reach back for the
// attach.
void DebuggerUnitTests::testASuccessfulRunDropsTheUnreadAttachStop()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard deleteEngine([engine] { delete engine; });
    engine->setRunParameters({});
    // A stop the inferior ran into selects the perspective, which only an
    // engine that was given its views has.
    engine->setDevice({});

    engine->start();
    reportAnAttachStopThatIsResumed(backend);

    emit backend->inferiorEvent(InferiorEvent::RunRequested);
    emit backend->inferiorEvent(InferiorEvent::RunOk);
    emit backend->inferiorEvent(InferiorEvent::SpontaneousStop);
    QCOMPARE(engine->state(), InferiorStopOk);

    // The stop the inferior ran into filled the views itself.
    backend->refreshes.clear();
    emit backend->inferiorEvent(InferiorEvent::RunRequested);
    emit backend->inferiorEvent(InferiorEvent::RunFailed);
    QVERIFY2(!backend->refreshes.contains(RefreshKind::FullStack),
             "a run that failed after the inferior had run read the attach stop again");
}

void DebuggerUnitTests::testTheDisassemblyMessageIsSaidOncePerStop()
{
    auto backend = new RecordingBackend;
    auto engine = new GenericDebuggerEngine("test", backend);
    const QScopeGuard cleanup([engine] {
        delete engine;
        EditorManager::closeAllEditors(false);
    });
    engine->setRunParameters({});

    StackFrame frame;
    frame.function = "ntdll!RtlAllocateHeap";
    frame.address = 0x1000;
    const Location loc(frame);
    QVERIFY(loc.canBeDisassembled());
    QVERIFY(!loc.hasDebugInfo());

    const auto mentions = [engine, &frame] {
        return engine->logWindow()->inputContents().count(frame.function);
    };

    engine->gotoLocation(loc);
    const int said = mentions();
    QVERIFY2(said > 0, qPrintable("nothing said before showing disassembly:\n"
                                  + engine->logWindow()->inputContents()));

    // An action that merely re-shows the current location does not repeat it.
    engine->gotoLocation(loc);
    QCOMPARE(mentions(), said);
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

void DebuggerUnitTests::testUnresolvedLegacyLanguagesSurviveASave()
{
    const FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile));
    QCOMPARE(projectManager.projects().size(), 1);
    BuildConfiguration * const bc = projectManager.projects().first()->activeBuildConfiguration();
    QVERIFY(bc);

    // "C++: Automatic, QML: Enabled", as Qt Creator 18 and earlier wrote it.
    Store legacy;
    legacy.insert("RunConfiguration.UseQmlDebugger", TriState::Enabled.toVariant());
    legacy.insert("RunConfiguration.UseCppDebuggerAuto", true);

    DebuggerRunConfigurationAspect aspect(bc);
    aspect.fromMap(legacy);

    Store saved;
    aspect.toMap(saved);

    // Nothing has picked a combo box item yet, so saving must leave the settings alone. Writing
    // out what they currently amount to would turn the automatic C++ choice into an explicit
    // one, and there is no way back from that on the next load.
    QCOMPARE(TriState::fromVariant(saved.value("RunConfiguration.UseCppDebugger")),
             TriState::Default);
    QCOMPARE(saved.value("RunConfiguration.UseCppDebuggerAuto").toBool(), true);
    QCOMPARE(TriState::fromVariant(saved.value("RunConfiguration.UseQmlDebugger")),
             TriState::Enabled);
    QVERIFY(!saved.contains("RunConfiguration.DebuggerLanguages"));
}

void DebuggerUnitTests::testLegacyQmlAndPythonKeepsBothLanguages()
{
    const FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile));
    QCOMPARE(projectManager.projects().size(), 1);
    BuildConfiguration * const bc = projectManager.projects().first()->activeBuildConfiguration();
    QVERIFY(bc);

    Store legacy;
    legacy.insert("RunConfiguration.UseQmlDebugger", TriState::Enabled.toVariant());
    legacy.insert("RunConfiguration.UsePythonDebugger", TriState::Enabled.toVariant());

    DebuggerRunConfigurationAspect aspect(bc);
    aspect.fromMap(legacy);

    QVERIFY(aspect.useQmlDebugger());
    QVERIFY(aspect.usePythonDebugger());
}

void DebuggerUnitTests::testJavaOnlyEnablesNoOtherDebugger()
{
    const FilePath proFile = m_tmpDir->absolutePath("simple/simple.pro");
    CppEditor::Tests::ProjectOpenerAndCloser projectManager;
    QVERIFY(projectManager.open(proFile));
    QCOMPARE(projectManager.projects().size(), 1);
    BuildConfiguration * const bc = projectManager.projects().first()->activeBuildConfiguration();
    QVERIFY(bc);

    Store stored;
    stored.insert("RunConfiguration.DebuggerLanguages", "Java");
    DebuggerRunConfigurationAspect aspect(bc);
    aspect.fromMap(stored);

    QVERIFY(aspect.useJavaDebugger());
    QVERIFY(!aspect.isAutomatic());
    // A C++ project, which "Automatic" would debug with the C++ debugger.
    QVERIFY(!aspect.useCppDebugger());
    QVERIFY(!aspect.useQmlDebugger());
    QVERIFY(!aspect.usePythonDebugger());

    Store saved;
    aspect.toMap(saved);
    QCOMPARE(saved.value("RunConfiguration.DebuggerLanguages").toString(), QString("Java"));
}

void DebuggerUnitTests::testAutomaticDebugsAJavaLaunchAsJava()
{
    const auto fixedUp = [](const QString &executable, DebuggerStartMode startMode) {
        DebuggerRunParameters rp;
        rp.setCppEngineType(GdbEngineType);
        rp.setUseTerminal(true);
        rp.setStartMode(startMode);
        rp.setInferiorExecutable(FilePath::fromString(executable));
        RunControl runControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        QTC_CHECK(rp.fixupParameters(&runControl));
        return rp;
    };

    const DebuggerRunParameters java = fixedUp("/nonexistent/java", NoStartMode);
    QVERIFY(java.isJavaDebugging());
    QVERIFY(!java.isCppDebugging());
    QVERIFY(!java.useTerminal());
    QVERIFY(fixedUp("/nonexistent/javaw", StartExternal).isJavaDebugging());
    // Attaching to the process of the virtual machine stays native debugging.
    QVERIFY(!fixedUp("/nonexistent/java", AttachToLocalProcess).isJavaDebugging());
    QVERIFY(!fixedUp("/nonexistent/javac", NoStartMode).isJavaDebugging());
}

void DebuggerUnitTests::testOverrideKitDebugger()
{
    const auto setOverride = [](const QString &value, EnvironmentItem::Operation operation) {
        Environment::modifySystemEnvironment(
            {EnvironmentItem("QTC_DEBUGGER_PATH", value, operation)});
    };

    const bool wasSet = qtcEnvironmentVariableIsSet("QTC_DEBUGGER_PATH");
    const QString oldValue = qtcEnvironmentVariable("QTC_DEBUGGER_PATH");
    const QScopeGuard restore([&] {
        setOverride(oldValue, wasSet ? EnvironmentItem::SetEnabled : EnvironmentItem::Unset);
    });
    setOverride({}, EnvironmentItem::Unset);

    // What fromRunControl() leaves for a kit without a debugger.
    const auto kitWithoutDebugger = [] {
        DebuggerRunParameters rp;
        rp.m_validationErrors.append("The kit does not have a debugger set.");
        rp.m_version = "kit's version";
        return rp;
    };

    DebuggerRunParameters rp = kitWithoutDebugger();
    rp.overrideKitDebugger("/sdk/gdb", GdbEngineType);
    QCOMPARE(rp.debugger().command.executable(), FilePath("/sdk/gdb"));
    QCOMPARE(rp.debugger().environment.value("LC_NUMERIC"), QString("C"));
    QCOMPARE(rp.cppEngineType(), GdbEngineType);
    QVERIFY(rp.version().isEmpty());
    QVERIFY(rp.m_validationErrors.isEmpty());

    // The run configuration does not debug C++.
    rp = kitWithoutDebugger();
    rp.m_cppDebuggingRequested = false;
    rp.overrideKitDebugger("/sdk/gdb", GdbEngineType);
    QCOMPARE(rp.cppEngineType(), NoEngineType);

    setOverride("/usr/bin/my-gdb", EnvironmentItem::SetEnabled);
    rp = kitWithoutDebugger();
    rp.overrideKitDebugger("/sdk/gdb", GdbEngineType);
    QCOMPARE(rp.debugger().command.executable(), FilePath("/usr/bin/my-gdb"));
}

// A backend that gets through a session as soon as it is asked to.
class SessionBackend final : public RecordingBackend
{
public:
    void start() override
    {
        report(InferiorEvent::EngineSetupOk, InferiorEvent::RunAndInferiorRunOk);
    }
    void execute(const ExecutionRequest &request) override
    {
        if (request.command == ExecutionCommand::Interrupt)
            report(InferiorEvent::StopOk);
    }
    void shutdownInferior(ShutdownMode) override { report(InferiorEvent::ShutdownFinished); }
    void shutdownEngine() override { report(InferiorEvent::EngineShutdownFinished); }

private:
    // Not from within the call: the engine changes its state after making it.
    void report(InferiorEvent first, std::optional<InferiorEvent> second = {})
    {
        QMetaObject::invokeMethod(this, [this, first, second] {
            emit inferiorEvent(first);
            if (second)
                emit inferiorEvent(*second);
        }, Qt::QueuedConnection);
    }
};

// A plugin with a debugger of its own brings the backend. Neither the type of
// the kit's debugger nor what is wrong with it matters then.
void DebuggerUnitTests::testPluginBringsTheBackend()
{
    Kit *kit = KitManager::defaultKit();
    QVERIFY(kit);

    const std::unique_ptr<RunControl> runControl(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    runControl->setKit(kit);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl.get());
    rp.setInferiorExecutable(FilePath::fromUserInput("/nonexistent/program"));
    // A Windows binary, a debugger that does not fit it, and does not work either.
    rp.setToolChainAbi(Abi(Abi::X86Architecture, Abi::WindowsOS, Abi::WindowsMsvc2022Flavor,
                           Abi::PEFormat, 64));
    rp.setCppEngineType(GdbEngineType);
    rp.m_validationErrors.append("The debugger of the kit is broken.");

    QPointer<SessionBackend> backend;
    DebuggerStartMode startMode = NoStartMode;
    rp.setEngineInterface("Plugin", [&backend, &startMode](const DebuggerRunParameters &params) {
        startMode = params.startMode();
        auto created = std::make_unique<SessionBackend>();
        backend = created.get();
        return created;
    });
    QVERIFY(rp.isCppDebugging());
    QCOMPARE(rp.cppEngineType(), NoEngineType);

    const QList<QPointer<DebuggerEngine>> before = EngineManager::engines();
    connect(runControl.get(), &RunControl::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    runControl->setRunRecipe(debuggerRecipe(runControl.get(), rp));
    runControl->start();

    // EngineManager holds the engines other tests left behind, too.
    const auto newEngine = [&before] {
        for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
            if (candidate && !before.contains(candidate))
                return candidate;
        }
        return QPointer<DebuggerEngine>();
    };
    QTRY_VERIFY(newEngine() && newEngine()->state() == InferiorRunOk);
    QVERIFY(backend);
    QCOMPARE(startMode, StartInternal);
    QCOMPARE(newEngine()->runParameters().engineInterfaceName(), QString("Plugin"));

    runControl->initiateStop();
    QTestEventLoop::instance().enterLoop(30);
    QVERIFY(!QTestEventLoop::instance().timeout());
}

// The plugin could not build its backend, for instance because a binary of
// its debugger is missing. That ends the run with a message.
void DebuggerUnitTests::testPluginFailsToBringTheBackend()
{
    Kit *kit = KitManager::defaultKit();
    QVERIFY(kit);

    const std::unique_ptr<RunControl> runControl(
        new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE));
    runControl->setKit(kit);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl.get());
    rp.setInferiorExecutable(FilePath::fromUserInput("/nonexistent/program"));
    rp.setStartMode(StartInternal);
    rp.setEngineInterface("Plugin", [](const DebuggerRunParameters &) {
        return std::unique_ptr<DebuggerEngineInterface>();
    });

    QStringList errors;
    connect(runControl.get(), &RunControl::appendMessage, runControl.get(),
            [&errors](const QString &message, OutputFormat format) {
        if (format == ErrorMessageFormat)
            errors.append(message);
    });
    const QList<QPointer<DebuggerEngine>> before = EngineManager::engines();
    bool stopped = false;
    connect(runControl.get(), &RunControl::stopped, runControl.get(),
            [&stopped] { stopped = true; });
    runControl->setRunRecipe(debuggerRecipe(runControl.get(), rp));
    runControl->start();
    QTRY_VERIFY(stopped);

    QVERIFY2(Utils::anyOf(errors, [](const QString &message) {
                 return message.contains("\"Plugin\" could not be created");
             }),
             qPrintable(errors.join('\n')));
    QCOMPARE(EngineManager::engines(), before);
}

// The backend a plugin brings debugs C++, so it is not used where C++ is not
// debugged.
void DebuggerUnitTests::testPluginBackendNeedsCppDebugging()
{
    const auto create = [](const DebuggerRunParameters &) {
        return std::make_unique<SessionBackend>();
    };

    DebuggerRunParameters rp;
    rp.m_cppDebuggingRequested = false;
    rp.setEngineInterface("Plugin", create);
    QVERIFY(!rp.isCppDebugging());

    rp = {};
    rp.setEngineInterface("Plugin", create);
    rp.setStartMode(AttachToQmlServer);
    QVERIFY(!rp.isCppDebugging());

    rp = {};
    rp.setStartMode(AttachToQmlServer);
    rp.setEngineInterface("Plugin", create);
    QVERIFY(!rp.isCppDebugging());

    rp = {};
    rp.setEngineInterface("Plugin", create);
    rp.setCppEngineType(NoEngineType);
    QVERIFY(!rp.isCppDebugging());
    QVERIFY(rp.engineInterfaceName().isEmpty());

    rp = {};
    rp.setEngineInterface("Plugin", create);
    rp.setInferiorExecutable(FilePath::fromString("/nonexistent/java"));
    RunControl runControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    QVERIFY(rp.fixupParameters(&runControl));
    QVERIFY(rp.isJavaDebugging());
    QVERIFY(!rp.isCppDebugging());
}

void DebuggerUnitTests::testNativeMixedEnvironmentVariableWins()
{
    const auto setOverride = [](const QString &value, EnvironmentItem::Operation operation) {
        Environment::modifySystemEnvironment(
            {EnvironmentItem("QTC_DEBUGGER_NATIVE_MIXED", value, operation)});
    };

    const bool wasSet = qtcEnvironmentVariableIsSet("QTC_DEBUGGER_NATIVE_MIXED");
    const QString oldValue = qtcEnvironmentVariable("QTC_DEBUGGER_NATIVE_MIXED");
    const QScopeGuard restore([&] {
        setOverride(oldValue, wasSet ? EnvironmentItem::SetEnabled : EnvironmentItem::Unset);
    });
    setOverride({}, EnvironmentItem::Unset);

    DebuggerRunParameters rp;
    rp.setCppEngineType(GdbEngineType);
    rp.setQmlDebugging(true);

    rp.setNativeMixedEnabled(true);
    QVERIFY(rp.isNativeMixedDebugging());
    rp.setNativeMixedEnabled(false);
    QVERIFY(!rp.isNativeMixedDebugging());

    setOverride("1", EnvironmentItem::SetEnabled);
    rp.setNativeMixedEnabled(false);
    QVERIFY(rp.isNativeMixedDebugging());

    setOverride("0", EnvironmentItem::SetEnabled);
    rp.setNativeMixedEnabled(true);
    QVERIFY(!rp.isNativeMixedDebugging());

    setOverride({}, EnvironmentItem::Unset);
    rp.setNativeMixedEnabled(true);
    QVERIFY(rp.isNativeMixedDebugging());
}

void DebuggerUnitTests::testCombinedEngineNeedsNoQmlChannel()
{
    const auto usesQmlChannel = [](bool nativeMixed) {
        DebuggerRunParameters rp;
        rp.setCppEngineType(GdbEngineType);
        rp.setQmlDebugging(true);
        rp.setNativeMixedEnabled(nativeMixed);
        RunControl runControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        rp.setupPortsGatherer(&runControl);
        QTC_CHECK(runControl.usesDebugChannel());
        return runControl.usesQmlChannel();
    };

    QVERIFY(usesQmlChannel(false));
    QVERIFY(!usesQmlChannel(true));
}

static const char s_parentlessObjectSource[] = R"(import QtQuick

Item {
    id: root
    property QtObject host: null
    property Component hostComponent: Component {
        QtObject {
            id: host
            property QtObject orphan: null
            property Component orphanComponent: Component {
                QtObject {
                    id: orphanObject
                    property int orphanValue: 7
                }
            }
            Component.onCompleted: host.orphan = orphanComponent.createObject(null)
        }
    }
    Component.onCompleted: root.host = hostComponent.createObject(null)
}
)";

// Parentless objects are fetched again whenever the object tree is rebuilt,
// which happens after each burst of object creations. Their properties must
// wait for the expansion, or the application keeps dumping them.
void DebuggerUnitTests::testQmlInspectorLeavesParentlessObjectsCollapsed()
{
    Kit *kit = kitWithAQt();
    if (!kit)
        QSKIP("This test needs a kit with a Qt to run qml from.");
    const FilePath qml = (QtSupport::QtKitAspect::qtVersion(kit)->binPath() / "qml")
                             .withExecutableSuffix();
    if (!qml.isExecutableFile())
        QSKIP("The Qt of this kit has no qml tool.");
    if (!settings().showQmlObjectTree())
        QSKIP("The QML object tree is switched off.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath source = FilePath::fromString(dir.path()) / "main.qml";
    QVERIFY(source.writeFileContents(QByteArray(s_parentlessObjectSource)));

    quint16 port = 0;
    {
        QTcpServer probe;
        QVERIFY(probe.listen(QHostAddress::LocalHost));
        port = probe.serverPort();
    }
    Process inferior;
    inferior.setCommand({qml, {QString("-qmljsdebugger=port:%1,block").arg(port),
                               source.nativePath()}});
    inferior.start();
    QVERIFY(inferior.waitForStarted());

    const QList<QPointer<DebuggerEngine>> before = EngineManager::engines();
    auto runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    QUrl server;
    server.setScheme(Utils::urlTcpScheme());
    server.setHost("127.0.0.1");
    server.setPort(port);
    rp.setQmlServer(server);
    rp.setStartMode(AttachToQmlServer);
    connect(runControl, &RunControl::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();

    const QScopeGuard stop([runControl, &inferior] {
        runControl->initiateStop();
        QTestEventLoop::instance().enterLoop(30);
        inferior.kill();
        inferior.waitForFinished();
    });

    QPointer<DebuggerEngine> engine;
    const auto findOrphan = [&engine, &before]() -> WatchItem * {
        for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
            if (candidate && !before.contains(candidate))
                engine = candidate;
        }
        if (!engine)
            return nullptr;
        WatchItem *inspect = engine->watchHandler()->findItem("inspect");
        if (!inspect)
            return nullptr;
        return inspect->findAnyChild([](TreeItem *item) {
            return static_cast<WatchItem *>(item)->name == "orphanObject";
        });
    };
    QTRY_VERIFY_WITH_TIMEOUT(findOrphan(), 30000);
    const QString orphanIName = findOrphan()->iname;
    const QString propertiesIName = orphanIName + ".[properties]";

    WatchHandler *watchHandler = engine->watchHandler();
    QVERIFY2(!watchHandler->findItem(propertiesIName),
             "a collapsed parentless object reported its properties anyway");

    watchHandler->fetchMore(orphanIName);
    QTRY_VERIFY_WITH_TIMEOUT(watchHandler->findItem(propertiesIName + ".orphanValue"), 30000);
}

// Objects made with createObject(null) have no parent, so the inspector hangs
// them directly below their engine. The test starts the churning only after
// the ticks: a new object makes the inspector fetch the tree again, which
// brings every value along whether or not its change came through.
static const char s_parentlessObjectsQml[] = R"QML(
import QtQuick
import QtQuick.Window

Window {
    id: root
    property int ticks: 0
    property int churns: 0
    property bool churning: false
    property bool spawning: false
    property var ticking
    property var churned
    property QtObject box: QtObject {
        id: holder
        property QtObject inner: QtObject { id: nested; property int tick: root.ticks }
    }

    Component {
        id: tickingComponent
        QtObject { id: tickingOrphan; property int tick: root.ticks }
    }
    Component {
        id: churnedComponent
        QtObject { id: churnedOrphan; property QtObject child: QtObject { id: orphanChild } }
    }
    Component {
        id: lateComponent
        QtObject { id: lateChild }
    }
    Timer {
        interval: 100
        repeat: true
        running: true
        onTriggered: ++root.ticks
    }
    Timer {
        interval: 50
        running: root.spawning
        onTriggered: {
            root.spawning = false
            lateComponent.createObject(holder)
        }
    }
    Timer {
        interval: 400
        repeat: true
        running: root.churning
        onTriggered: {
            if (root.churned)
                root.churned.destroy()
            root.churned = churnedComponent.createObject(null)
            ++root.churns
        }
    }
    Component.onCompleted: ticking = tickingComponent.createObject(null)
}
)QML";

// The item Locals shows for a property of the inspected object with the given
// QML id. Asks for the properties when they are not there yet.
static WatchItem *inspectedItem(DebuggerEngine *engine, const QString &id,
                                const QString &property)
{
    WatchHandler *handler = engine->watchHandler();
    const WatchItem *root = handler->findItem("inspect");
    if (!root)
        return nullptr;
    const WatchItem *object = root->findAnyChild([&id](TreeItem *item) {
        return static_cast<WatchItem *>(item)->name == id;
    });
    if (!object)
        return nullptr;
    if (WatchItem *item = handler->findItem(object->iname + ".[properties]." + property))
        return item;
    handler->fetchMore(object->iname);
    return nullptr;
}

static std::optional<int> inspectedProperty(DebuggerEngine *engine, const QString &id,
                                            const QString &property)
{
    if (const WatchItem *item = inspectedItem(engine, id, property)) {
        bool ok = false;
        const int value = item->value.toInt(&ok);
        if (ok)
            return value;
    }
    return {};
}

void DebuggerUnitTests::testInspectorFollowsParentlessObjects()
{
    Kit *kit = kitWithAQt();
    if (!kit)
        QSKIP("This test needs a kit with a Qt to run qml from.");
    const FilePath qml = QtSupport::QtKitAspect::qtVersion(kit)->qmlRuntimeFilePath();
    if (!qml.isExecutableFile())
        QSKIP("The Qt of the kit has no qml runtime.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const FilePath mainQml = FilePath::fromString(dir.path()) / "Main.qml";
    QVERIFY(mainQml.writeFileContents(s_parentlessObjectsQml));

    // Nothing here can answer the dialog about breakpoints that do not apply.
    const bool warnedAboutBreakpoints = settings().showUnsupportedBreakpointWarning();
    settings().showUnsupportedBreakpointWarning.setValue(false);
    const QScopeGuard restore([warnedAboutBreakpoints] {
        settings().showUnsupportedBreakpointWarning.setValue(warnedAboutBreakpoints);
    });

    const QList<QPointer<DebuggerEngine>> before = EngineManager::engines();
    QPointer<RunControl> runControl = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
    runControl->setKit(kit);
    DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
    rp.setCppEngineType(NoEngineType);
    rp.setQmlDebugging(true);
    rp.setInferior(ProcessRunData{CommandLine{qml, {mainQml.nativePath()}}, mainQml.parentDir()});
    rp.setInferiorEnvironment(Environment::systemEnvironment());
    rp.setStartMode(StartExternal);
    connect(runControl, &RunControl::stopped,
            &QTestEventLoop::instance(), &QTestEventLoop::exitLoop);
    runControl->setRunRecipe(debuggerRecipe(runControl, rp));
    runControl->start();
    const QScopeGuard stop([runControl] {
        if (runControl && runControl->isRunning()) {
            runControl->initiateStop();
            QTestEventLoop::instance().enterLoop(30);
        }
    });

    QPointer<DebuggerEngine> engine;
    const bool running = QTest::qWaitFor([&engine, &before] {
        for (const QPointer<DebuggerEngine> &candidate : EngineManager::engines()) {
            if (candidate && !before.contains(candidate)
                && candidate->objectName() == "QmlEngine") {
                engine = candidate;
            }
        }
        return engine && engine->state() == InferiorRunOk;
    }, 60000);
    QVERIFY2(running, "the QML session never came up");
    const QmlInspectorAgent &agent = static_cast<QmlEngine *>(engine.data())->inspectorAgent();

    std::optional<int> firstTick;
    QVERIFY2(QTest::qWaitFor([&] {
        return bool(firstTick = inspectedProperty(engine, "tickingOrphan", "tick"));
    }, 30000), "the parentless object never showed its properties");
    // Changes come in batches, the root's and the orphan's of one tick together.
    std::optional<int> ticks;
    QVERIFY2(QTest::qWaitFor([&] {
        ticks = inspectedProperty(engine, "root", "ticks");
        return ticks && *ticks >= *firstTick + 3;
    }, 10000), "the ticks of the root object never moved in Locals");
    const std::optional<int> tick = inspectedProperty(engine, "tickingOrphan", "tick");
    QVERIFY2(tick && *tick != *firstTick,
             "a change of the parentless object never reached Locals");

    // An object two levels down an expanded subtree must keep its watch while
    // the churning rebuilds the tree.
    QVERIFY2(QTest::qWaitFor([&] {
        return inspectedItem(engine, "holder", "inner")
               && inspectedProperty(engine, "nested", "tick");
    }, 30000), "the nested object never showed its properties");
    const int nestedId = inspectedItem(engine, "nested", "tick")->id;
    const quint32 nestedWatch = agent.m_objectWatches.value(nestedId);
    QVERIFY(nestedWatch);

    // Parentless objects live below their engine, not at the top level.
    const WatchItem *inspectRoot = engine->watchHandler()->findItem("inspect");
    QVERIFY(inspectRoot);
    const auto named = [](const QString &name) {
        return [name](TreeItem *item) { return static_cast<WatchItem *>(item)->name == name; };
    };
    const WatchItem *orphan = inspectRoot->findAnyChild(named("tickingOrphan"));
    QVERIFY(orphan);
    QVERIFY(orphan->parent() != inspectRoot);
    QVERIFY(orphan->parent()->parent() == inspectRoot);

    // A child of an expanded object is inserted at once, without waiting for
    // the throttled rebuild. The rebuild starts by emptying the inspector tree,
    // and its answer comes after that of the direct fetch, so the child must
    // show up before the tree is emptied. Let the rebuilds from the startup
    // finish first.
    QVERIFY2(QTest::qWaitFor([&agent] {
        return !agent.m_delayQueryTimer.isActive() && agent.m_rootContextQueryIds.isEmpty();
    }, 30000), "the rebuild never finished");
    const auto lateChildShown = [&] {
        const WatchItem *holder = inspectRoot->findAnyChild(named("holder"));
        return holder && holder->findAnyChild(named("lateChild"));
    };
    WatchModelBase *model = engine->watchHandler()->model();
    bool cleared = false;
    bool shownBeforeClear = false;
    QObject guard;
    connect(model, &QAbstractItemModel::rowsInserted, &guard, [&] {
        if (!cleared && lateChildShown())
            shownBeforeClear = true;
    });
    connect(model, &QAbstractItemModel::rowsAboutToBeRemoved, &guard,
            [&](const QModelIndex &parent) {
                if (model->itemForIndex(parent) == inspectRoot)
                    cleared = true;
            });
    WatchItem *spawning = inspectedItem(engine, "root", "spawning");
    QVERIFY(spawning);
    engine->assignValueInDebugger(spawning, "spawning", "true");
    QVERIFY2(QTest::qWaitFor([&] { return shownBeforeClear || cleared; }, 30000),
             "neither the child of the expanded object nor a rebuild showed up");
    QVERIFY2(shownBeforeClear, "the child of the expanded object waited for the rebuild");

    // A rebuild that comes due while the answers to the previous one are still
    // on their way must wait for them instead of dropping them.
    const auto rebuildDone = [&agent] {
        return !agent.m_delayQueryTimer.isActive() && !agent.m_rebuildPending
               && agent.m_rootContextQueryIds.isEmpty();
    };
    QVERIFY2(QTest::qWaitFor(rebuildDone, 30000), "the rebuild never finished");
    auto &mutableAgent = const_cast<QmlInspectorAgent &>(agent);
    mutableAgent.queryEngineContext();
    const QList<quint32> pendingQueries = agent.m_rootContextQueryIds;
    QVERIFY(!pendingQueries.isEmpty());
    mutableAgent.rebuildObjectTree();
    QCOMPARE(agent.m_rootContextQueryIds, pendingQueries);
    QVERIFY2(QTest::qWaitFor([&agent] { return agent.m_rootContextQueryIds.isEmpty(); }, 30000),
             "the pending rebuild never finished");
    QVERIFY2(agent.m_delayQueryTimer.isActive(), "the rebuild that came due was dropped");
    const qint64 rebuilt = agent.m_lastRebuild.msecsSinceReference();
    QVERIFY2(QTest::qWaitFor([&] {
        return rebuildDone() && agent.m_lastRebuild.msecsSinceReference() != rebuilt;
    }, 30000), "the rebuild that came due was never run");

    // Each churn destroys an object whose child is watched as well. The watches
    // of the destroyed ones must go away. How many watches the live objects
    // need depends on how often the tree gets rebuilt, so follow the watches
    // of one churned object rather than the count.
    WatchItem *churning = nullptr;
    QVERIFY2(QTest::qWaitFor([&] {
        return bool(churning = inspectedItem(engine, "root", "churning"));
    }, 30000), "the root object never showed its properties again");
    engine->assignValueInDebugger(churning, "churning", "true");
    QSet<int> churnedWatches;
    QVERIFY2(QTest::qWaitFor([&] {
        const WatchItem *root = engine->watchHandler()->findItem("inspect");
        const WatchItem *churned = root ? root->findAnyChild([](TreeItem *item) {
            return static_cast<WatchItem *>(item)->name == "churnedOrphan";
        }) : nullptr;
        if (!churned || !agent.m_objectWatches.contains(churned->id))
            return false;
        churnedWatches.insert(churned->id);
        churned->forAllChildren([&](WatchItem *item) {
            if (agent.m_objectWatches.contains(item->id))
                churnedWatches.insert(item->id);
        });
        return true;
    }, 30000), "the churned object was never watched");
    std::optional<int> firstChurn;
    QVERIFY2(QTest::qWaitFor([&] {
        firstChurn = inspectedProperty(engine, "root", "churns");
        return firstChurn && *firstChurn > 0;
    }, 30000), "the churning never started");
    QVERIFY2(QTest::qWaitFor([&] {
        const std::optional<int> churn = inspectedProperty(engine, "root", "churns");
        return churn && *churn >= *firstChurn + 16;
    }, 30000), "the churn count stopped moving in Locals");
    for (const int id : std::as_const(churnedWatches)) {
        if (agent.m_objectWatches.contains(id))
            QFAIL(qPrintable(QString("the destroyed object %1 kept its watch").arg(id)));
    }
    QCOMPARE(agent.m_objectWatches.value(nestedId), nestedWatch);

    // The churning goes on, and a collapsed engine item must stay collapsed
    // while the new objects are added below it.
    const WatchItem *ticking = inspectRoot->findAnyChild(named("tickingOrphan"));
    QVERIFY(ticking);
    const QString engineIName = ticking->parent()->iname;
    model->setData(model->indexForItem(ticking->parent()), false, LocalsExpandedRole);
    QVERIFY(!engine->watchHandler()->isExpandedIName(engineIName));
    for (int i = 0; i < 2; ++i) {
        const qint64 rebuilt = agent.m_lastRebuild.msecsSinceReference();
        QVERIFY2(QTest::qWaitFor([&] {
            return agent.m_lastRebuild.msecsSinceReference() != rebuilt;
        }, 30000), "the tree was not rebuilt while the objects kept coming");
    }
    QVERIFY2(!engine->watchHandler()->isExpandedIName(engineIName),
             "the collapsed engine item was expanded again");
    QVERIFY2(QTest::qWaitFor([&] {
        const WatchItem *engineItem = engine->watchHandler()->findItem(engineIName);
        return engineItem && engineItem->findAnyChild(named("churnedOrphan"));
    }, 30000), "no new object showed up below the collapsed engine item");
}

QObject *createDebuggerTest()
{
    return new DebuggerUnitTests;
}

} // Debugger::Internal

#include "debuggertest.moc"

#endif // WITH_TESTS
