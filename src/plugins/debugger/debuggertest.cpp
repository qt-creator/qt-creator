// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "debuggertest.h"

#include "debuggercore.h"
#include "debuggerengineinterface.h"
#include "debuggeritem.h"
#include "debuggerruncontrol.h"
#include "debuggersourcepathmappingwidget.h"
#include "enginemanager.h"
#include "gdb/gdbengine.h"
#include "registerhandler.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/filepath.h>
#include <utils/hostosinfo.h>

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
    void testMapsAnEmptyFileNameToNothing();

    void testQtBuildSourceRoots_data();
    void testQtBuildSourceRoots();

    void testDebugInfoDirectory();
    void testDebugInfoFile();
    void testMergePlatformQtPath();

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

QObject *createDebuggerTest()
{
    return new DebuggerUnitTests;
}

} // Debugger::Internal

#include "debuggertest.moc"

#endif // WITH_TESTS
