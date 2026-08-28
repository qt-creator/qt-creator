// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "debuggertest.h"

#include "debuggercore.h"
#include "debuggerengineinterface.h"
#include "debuggeritem.h"
#include "debuggerruncontrol.h"
#include "gdb/gdbengine.h"
#include "registerhandler.h"

#include <coreplugin/editormanager/editormanager.h>

#include <cppeditor/cpptoolstestcase.h>

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/filepath.h>

#include <QTest>
#include <QSignalSpy>
#include <QTestEventLoop>

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

    void testRegisterValue_data();
    void testRegisterValue();

    void testInferiorStartData();

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

    QTestEventLoop::instance().enterLoop(5);
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

QObject *createDebuggerTest()
{
    return new DebuggerUnitTests;
}

} // Debugger::Internal

#include "debuggertest.moc"

#endif // WITH_TESTS
