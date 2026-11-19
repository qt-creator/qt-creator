// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <debugger/debuggerengineinterface.h>
#include <debugger/debuggerprotocol.h>
#include <debugger/disassemblerlines.h>
#include <debugger/registerhandler.h>
#include <debugger/uvsc/uvscclient.h>
#include <debugger/uvsc/uvscimpl.h>

#include <utils/filepath.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Utils;

static GdbMi make(const QString &name, const QString &data = {},
                  GdbMi::Type type = GdbMi::Const)
{
    GdbMi item;
    item.m_name = name;
    item.m_data = data;
    item.m_type = type;
    return item;
}

static GdbMi tuple(const QString &name = {})
{
    return make(name, {}, GdbMi::Tuple);
}

static GdbMi list(const QString &name = {})
{
    return make(name, {}, GdbMi::List);
}

// Stands in for uVision: answers from what the test put in, and keeps what it was asked.
class FakeUvscClient final : public UvscClientInterface
{
public:
    void version(QString &uvscVersion, QString &uvsockVersion) final
    {
        uvscVersion = "1.0";
        uvsockVersion = "2.0";
    }

    bool connectSession(qint32 uvscPort) final { port = uvscPort; return call("connect"); }
    void disconnectSession() final { call("disconnect"); }
    bool startSession(bool) final { return call("startSession"); }
    bool stopSession() final { return call("stopSession"); }

    bool executeStepOver() final { return call("stepOver"); }
    bool executeStepIn() final { return call("stepIn"); }
    bool executeStepOut() final { return call("stepOut"); }
    bool executeStepInstruction() final { return call("stepInstruction"); }

    bool startExecution() final { return call("startExecution"); }
    bool stopExecution() final { return call("stopExecution"); }

    bool fetchThreads(bool, GdbMi &data) final
    {
        GdbMi frame = tuple("frame");
        frame.addChild(make("addr", "0x8000210"));
        frame.addChild(make("func", "main"));
        frame.addChild(make("fullname", "C:/src/main.c"));
        frame.addChild(make("line", "25"));
        GdbMi thread = tuple();
        thread.addChild(make("name", "main"));
        thread.addChild(make("id", "7"));
        thread.addChild(make("state", "Running"));
        thread.addChild(frame);
        GdbMi threads = list("threads");
        threads.addChild(thread);
        data = tuple("data");
        data.addChild(threads);
        return call("fetchThreads");
    }

    bool fetchStackFrames(quint32 taskId, quint64 address, GdbMi &data) final
    {
        stackTaskId = taskId;
        stackAddress = address;
        GdbMi frames = list("frames");
        for (int level = 1; level <= 2; ++level) {
            GdbMi frame = tuple("frame");
            frame.addChild(make("level", QString::number(level)));
            frame.addChild(make("address", QString("0x%1").arg(0x8000200 + level, 0, 16)));
            frame.addChild(make("context", ""));
            frame.addChild(make("function", level == 1 ? "main" : "foo"));
            frame.addChild(make("file", "C:/src/main.c"));
            frame.addChild(make("line", QString::number(20 + level)));
            frames.addChild(frame);
        }
        GdbMi stack = tuple("stack");
        stack.addChild(frames);
        data = tuple("result");
        data.addChild(stack);
        return call("fetchStackFrames");
    }

    bool fetchRegisters(Registers &result) final
    {
        Register reg;
        reg.name = "R0";
        reg.size = 4;
        reg.kind = FlagRegister;
        reg.description = "Core";
        reg.value.v.u32[0] = 0x1234;
        reg.value.known = true;
        result[3] = reg;
        return call("fetchRegisters");
    }

    bool fetchLocals(const QStringList &expandedLocalINames, qint32 taskId, qint32 frameId,
                     GdbMi &data) final
    {
        expandedLocals = expandedLocalINames;
        localsTaskId = taskId;
        localsFrameId = frameId;
        GdbMi local = tuple();
        local.addChild(make("iname", "local.a"));
        data.addChild(local);
        return call("fetchLocals");
    }

    bool fetchWatchers(const QStringList &expandedWatcherINames,
                       const std::vector<std::pair<QString, QString>> &rootWatchers,
                       GdbMi &data) final
    {
        expandedWatchers = expandedWatcherINames;
        watchers = rootWatchers;
        GdbMi watcher = tuple();
        watcher.addChild(make("iname", "watch.0"));
        data.addChild(watcher);
        return call("fetchWatchers");
    }

    bool fetchMemory(quint64 address, QByteArray &data) final
    {
        memoryAddresses.append(address);
        const QByteArray bytes = memory.mid(address - memoryBase, data.size());
        data = bytes;
        return call("fetchMemory");
    }

    bool changeMemory(quint64 address, const QByteArray &data) final
    {
        changedAddress = address;
        changedData = data;
        return call("changeMemory");
    }

    bool disassemblyAddress(quint64 address, QByteArray &result) final
    {
        disassembledAddress = address;
        result = disassembly;
        return call("disassemblyAddress");
    }

    bool setRegisterValue(int index, const QString &value) final
    {
        registerIndex = index;
        registerValue = value;
        return call("setRegisterValue");
    }

    bool setLocalValue(qint32, qint32, qint32, const QString &) final
    {
        return call("setLocalValue");
    }

    bool setWatcherValue(qint32, const QString &) final { return call("setWatcherValue"); }

    bool createBreakpoint(const QString &exp, quint32 &tickMark, quint64 &address,
                          quint32 &line, QString &function, QString &fileName) final
    {
        breakpointExpression = exp;
        tickMark = 42;
        address = 0x8000210;
        line = 25;
        function = "main";
        fileName = "C:/src/main.c";
        return call("createBreakpoint");
    }

    bool deleteBreakpoint(quint32 tickMark) final
    {
        breakpointTickMark = tickMark;
        return call("deleteBreakpoint");
    }

    bool enableBreakpoint(quint32 tickMark) final
    {
        breakpointTickMark = tickMark;
        return call("enableBreakpoint");
    }

    bool disableBreakpoint(quint32 tickMark) final
    {
        breakpointTickMark = tickMark;
        return call("disableBreakpoint");
    }

    bool openProject(const FilePath &projectFile) final
    {
        openedProject = projectFile;
        return call("openProject");
    }

    void closeProject() final { call("closeProject"); }
    bool setProjectDebugTarget(bool) final { return call("setProjectDebugTarget"); }

    bool setProjectOutputTarget(const FilePath &outputFile) final
    {
        outputTarget = outputFile;
        return call("setProjectOutputTarget");
    }

    UvscError error() const final { return initError; }
    QString errorString() const final { return "fake failure"; }

    bool call(const QString &name)
    {
        calls.append(name);
        return !failing.contains(name);
    }

    QStringList calls;
    QSet<QString> failing;
    UvscError initError = NoError;
    qint32 port = 0;
    quint32 stackTaskId = 0;
    quint64 stackAddress = 0;
    QStringList expandedLocals;
    QStringList expandedWatchers;
    std::vector<std::pair<QString, QString>> watchers;
    qint32 localsTaskId = -1;
    qint32 localsFrameId = -1;
    quint64 memoryBase = 0;
    QByteArray memory;
    QList<quint64> memoryAddresses;
    quint64 changedAddress = 0;
    QByteArray changedData;
    quint64 disassembledAddress = 0;
    QByteArray disassembly;
    int registerIndex = -1;
    QString registerValue;
    QString breakpointExpression;
    quint32 breakpointTickMark = 0;
    FilePath openedProject;
    FilePath outputTarget;
};

// Drives UvscImpl the way GenericDebuggerEngine does, and keeps what it answers.
class DebuggerBackend : public QObject
{
public:
    explicit DebuggerBackend(UvscImplStartData startData)
    {
        startData.createClient = [this](const FilePath &) {
            auto client = std::make_unique<FakeUvscClient>();
            client->initError = initError;
            client->failing = failing;
            this->client = client.get();
            return client;
        };
        engine.reset(new UvscImpl(startData));
        connect(engine.get(), &DebuggerEngineInterface::inferiorEvent, this,
                [this](InferiorEvent event) { events.append(event); });
        connect(engine.get(), &DebuggerEngineInterface::message, this,
                [this](const QString &text, int, int) { log.append(text); });
        connect(engine.get(), &DebuggerEngineInterface::startFailed, this,
                [this](const QString &, const QString &message, const Key &) {
            startFailures.append(message);
        });
        connect(engine.get(), &DebuggerEngineInterface::refreshDataReceived, this,
                [this](quint64 requestId, RefreshKind, const GdbMi &data) {
            refreshes.insert(requestId, data);
        });
        connect(engine.get(), &DebuggerEngineInterface::breakpointEvent, this,
                [this](quint64 requestId, BreakpointOp, bool ok, const GdbMi &data) {
            breakpointAnswers.insert(requestId, {ok, data});
        });
        connect(engine.get(), &DebuggerEngineInterface::memoryRead, this,
                [this](quint64, const MemoryReadResult &result) { memoryReads.append(result); });
        connect(engine.get(), &DebuggerEngineInterface::disassemblyReceived, this,
                [this](quint64, const DisassemblerLines &lines) { disassembled = lines; });
    }

    ~DebuggerBackend() override
    {
        engine->disconnect();
        if (QTest::currentTestFailed())
            qWarning().noquote() << "The engine logged:\n" + log.join('\n');
    }

    bool setUp()
    {
        engine->start();
        if (!client || events.contains(InferiorEvent::EngineSetupFailed))
            return false;
        emit client->projectClosed();
        return events.contains(InferiorEvent::RunAndInferiorStopOk);
    }

    void execute(ExecutionCommand command, bool flag = false)
    {
        ExecutionRequest request;
        request.command = command;
        request.flag = flag;
        engine->execute(request);
    }

    void start() { engine->start(); }
    void refresh(const RefreshRequest &request) { engine->refresh(request); }
    void activateFrame(int index) { engine->activateFrame(index); }
    void changeBreakpoint(const BreakpointChangeRequest &request)
    {
        engine->changeBreakpoint(request);
    }

    void accessMemory(MemoryOp op, quint64 address, quint64 length, const QByteArray &data = {})
    {
        engine->accessMemory(op, 1, address, length, data);
    }

    void fetchDisassembly(quint64 address)
    {
        engine->fetchDisassembly(1, address, {}, {}, DisassemblyFlavor::Att);
    }

    void setRegisterValue(const QString &name, const QString &value)
    {
        engine->setRegisterValue(name, value);
    }

    void setPeripheralRegisterValue(quint64 address, quint64 value)
    {
        engine->setPeripheralRegisterValue(address, value);
    }

    std::unique_ptr<DebuggerEngineInterface> engine;
    FakeUvscClient *client = nullptr;
    UvscClientInterface::UvscError initError = UvscClientInterface::NoError;
    QSet<QString> failing;
    QList<InferiorEvent> events;
    QStringList log;
    QStringList startFailures;
    QMultiHash<quint64, GdbMi> refreshes;
    QHash<quint64, std::pair<bool, GdbMi>> breakpointAnswers;
    QList<MemoryReadResult> memoryReads;
    DisassemblerLines disassembled;
};

class tst_uvsc : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void setupSucceeds();
    void setupFails_data();
    void setupFails();
    void stepsFromTheRootFrameByInstruction_data();
    void stepsFromTheRootFrameByInstruction();
    void stopsMapToEvents();
    void interruptStops();
    void insertsAndTogglesBreakpoints();
    void insertsDisabledBreakpoint();
    void removesBreakpoint();
    void passesExpandedLocalsAndWatchers();
    void reportsRegisters();
    void reportsPeripheralRegisters();
    void reportsThreadsAndStack();
    void reportsModules();
    void parsesDisassembly();
    void readsMemory();

private:
    UvscImplStartData startData() const;

    QTemporaryDir m_dir;
};

void tst_uvsc::initTestCase()
{
    QVERIFY(m_dir.isValid());
    const FilePath root = FilePath::fromString(m_dir.path());
    const auto touch = [](const FilePath &file) {
        QVERIFY(file.parentDir().ensureWritableDir());
        QVERIFY(file.writeFileContents("x"));
    };
    touch(root / "UV4/UV4.exe");
    touch(root / "proj/my-app.uvprojx");
    touch(root / "proj/my-app.uvoptx");
    touch(root / "proj/out/my-app.axf");
}

UvscImplStartData tst_uvsc::startData() const
{
    const FilePath root = FilePath::fromString(m_dir.path());
    UvscImplStartData data;
    data.uVisionExecutable = root / "UV4/UV4.exe";
    data.remoteChannel = "tcp://localhost:5101";
    data.projectFile = root / "proj/my-app.uvprojx";
    data.optionsFile = root / "proj/my-app.uvoptx";
    data.inferiorExecutable = root / "proj/out/my-app.axf";
    return data;
}

void tst_uvsc::setupSucceeds()
{
    const FilePath guix = startData().projectFile.parentDir() / "my-app.uvguix.someone";
    QVERIFY(guix.writeFileContents("x"));

    DebuggerBackend backend(startData());
    backend.start();
    QVERIFY(backend.client);
    QCOMPARE(backend.client->port, 5101);
    QCOMPARE(backend.client->outputTarget, FilePath::fromString("out/my-app.axf"));
    QCOMPARE(backend.client->calls.last(), "closeProject");
    QVERIFY(backend.events.isEmpty());

    QObject::connect(backend.engine.get(), &DebuggerEngineInterface::inferiorEvent, &backend,
                     [&backend](InferiorEvent event) {
        if (event == InferiorEvent::EngineSetupOk)
            backend.client->calls.append("EngineSetupOk");
    });
    emit backend.client->projectClosed();
    QCOMPARE(backend.events,
             QList<InferiorEvent>({InferiorEvent::EngineSetupOk,
                                   InferiorEvent::RunAndInferiorStopOk}));
    // The breakpoints are claimed on EngineSetupOk, so the session runs by then.
    const QStringList &calls = backend.client->calls;
    const QStringList tail = calls.mid(calls.indexOf("closeProject"));
    QCOMPARE(tail, QStringList({"closeProject", "openProject", "startSession", "fetchThreads",
                                "EngineSetupOk"}));
    QVERIFY(!guix.exists());

    // A later close is uVision's own, not the setup's.
    emit backend.client->projectClosed();
    QCOMPARE(backend.events.size(), 2);
}

void tst_uvsc::setupFails_data()
{
    QTest::addColumn<QString>("channel");
    QTest::addColumn<bool>("removeProject");
    QTest::addColumn<int>("initError");
    QTest::addColumn<QString>("failing");

    QTest::newRow("no port") << "tcp://localhost" << false << 0 << "";
    QTest::newRow("library") << "tcp://localhost:5101" << false
                             << int(UvscClientInterface::InitializationError) << "";
    QTest::newRow("connect") << "tcp://localhost:5101" << false << 0 << "connect";
    QTest::newRow("open") << "tcp://localhost:5101" << false << 0 << "openProject";
    QTest::newRow("debug target") << "tcp://localhost:5101" << false << 0
                                  << "setProjectDebugTarget";
    QTest::newRow("output") << "tcp://localhost:5101" << false << 0 << "setProjectOutputTarget";
    QTest::newRow("missing project") << "tcp://localhost:5101" << true << 0 << "";
}

void tst_uvsc::setupFails()
{
    QFETCH(QString, channel);
    QFETCH(bool, removeProject);
    QFETCH(int, initError);
    QFETCH(QString, failing);

    UvscImplStartData data = startData();
    data.remoteChannel = channel;
    if (removeProject)
        data.projectFile = data.projectFile.parentDir() / "missing.uvprojx";
    DebuggerBackend backend(data);
    backend.initError = UvscClientInterface::UvscError(initError);
    if (!failing.isEmpty())
        backend.failing.insert(failing);

    backend.start();
    QCOMPARE(backend.events, QList<InferiorEvent>({InferiorEvent::EngineSetupFailed}));
    QCOMPARE(backend.startFailures.size(), 1);
}

void tst_uvsc::stepsFromTheRootFrameByInstruction_data()
{
    QTest::addColumn<int>("command");
    QTest::addColumn<int>("frameLevel");
    QTest::addColumn<bool>("byInstruction");
    QTest::addColumn<QString>("expected");

    const int over = int(ExecutionCommand::StepOver);
    const int in = int(ExecutionCommand::StepIn);
    const int out = int(ExecutionCommand::StepOut);
    QTest::newRow("over in root") << over << 1 << false << "stepInstruction";
    QTest::newRow("over in callee") << over << 2 << false << "stepOver";
    QTest::newRow("over by instruction") << over << 2 << true << "stepInstruction";
    QTest::newRow("in in root") << in << 1 << false << "stepInstruction";
    QTest::newRow("in in callee") << in << 2 << false << "stepIn";
    QTest::newRow("out in callee") << out << 2 << false << "stepOut";
}

void tst_uvsc::stepsFromTheRootFrameByInstruction()
{
    QFETCH(int, command);
    QFETCH(int, frameLevel);
    QFETCH(bool, byInstruction);
    QFETCH(QString, expected);

    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.activateFrame(frameLevel);
    backend.events.clear();
    backend.execute(ExecutionCommand(command), byInstruction);
    QCOMPARE(backend.client->calls.last(), expected);
    QCOMPARE(backend.events, QList<InferiorEvent>({InferiorEvent::RunRequested}));

    // A step that is over before uVision says it started is still a run.
    emit backend.client->executionStopped();
    QCOMPARE(backend.events,
             QList<InferiorEvent>({InferiorEvent::RunRequested, InferiorEvent::RunOk,
                                   InferiorEvent::SpontaneousStop}));
}

void tst_uvsc::stopsMapToEvents()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.events.clear();

    backend.execute(ExecutionCommand::Continue);
    emit backend.client->executionStarted();
    emit backend.client->executionStopped();
    QCOMPARE(backend.events,
             QList<InferiorEvent>({InferiorEvent::RunRequested, InferiorEvent::RunOk,
                                   InferiorEvent::SpontaneousStop}));

    // uVision runs the target on its own, from its own window.
    backend.events.clear();
    emit backend.client->executionStarted();
    QCOMPARE(backend.events,
             QList<InferiorEvent>({InferiorEvent::RunRequested, InferiorEvent::RunOk}));
    emit backend.client->executionStopped();
    emit backend.client->executionStopped();
    QCOMPARE(backend.events.size(), 3);
    QCOMPARE(backend.events.last(), InferiorEvent::SpontaneousStop);

    backend.events.clear();
    backend.client->failing.insert("startExecution");
    backend.execute(ExecutionCommand::Continue);
    QCOMPARE(backend.events,
             QList<InferiorEvent>({InferiorEvent::RunRequested, InferiorEvent::RunFailed}));
}

void tst_uvsc::interruptStops()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.execute(ExecutionCommand::Continue);
    emit backend.client->executionStarted();
    backend.events.clear();

    backend.execute(ExecutionCommand::Interrupt);
    QCOMPARE(backend.client->calls.last(), "stopExecution");
    emit backend.client->executionStopped();
    QCOMPARE(backend.events, QList<InferiorEvent>({InferiorEvent::StopOk}));
}

static BreakpointChangeRequest lineBreakpoint(BreakpointOp op, quint64 requestId, bool enabled)
{
    BreakpointChangeRequest request;
    request.op = op;
    request.requestId = requestId;
    request.params.type = BreakpointByFileAndLine;
    request.params.fileName = FilePath::fromString("C:/src/main.c");
    request.params.textPosition.line = 25;
    request.params.enabled = enabled;
    if (op != BreakpointOp::Insert)
        request.responseId = "42";
    return request;
}

static QString enabledIn(const std::pair<bool, GdbMi> &answer)
{
    if (answer.second.childCount() != 1)
        return "<no breakpoint data>";
    return answer.second.childAt(0)["enabled"].data();
}

void tst_uvsc::insertsAndTogglesBreakpoints()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());

    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Insert, 1, true));
    QCOMPARE(backend.client->breakpointExpression, "\\\\my_app\\C:/src/main.c\\25");
    QVERIFY(backend.breakpointAnswers.contains(1));
    const auto [ok, data] = backend.breakpointAnswers.value(1);
    QVERIFY(ok);
    QCOMPARE(data.childCount(), 1);
    const GdbMi bkpt = data.childAt(0);
    QCOMPARE(bkpt["number"].data(), "42");
    QCOMPARE(bkpt["addr"].data(), "0x8000210");
    QCOMPARE(bkpt["line"].data(), "25");
    QCOMPARE(bkpt["fullname"].data(), "C:/src/main.c");
    QCOMPARE(bkpt["func"].data(), "main");
    QCOMPARE(bkpt["enabled"].data(), "y");

    // The model takes the enabled state from the answer only.
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Update, 2, false));
    QCOMPARE(backend.client->calls.last(), "disableBreakpoint");
    QCOMPARE(backend.client->breakpointTickMark, 42u);
    QVERIFY(backend.breakpointAnswers.value(2).first);
    QCOMPARE(enabledIn(backend.breakpointAnswers.value(2)), "n");

    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Update, 3, true));
    QCOMPARE(backend.client->calls.last(), "enableBreakpoint");
    QCOMPARE(enabledIn(backend.breakpointAnswers.value(3)), "y");

    // Nothing to tell uVision when the state did not change.
    const int callCount = backend.client->calls.size();
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Update, 4, true));
    QCOMPARE(backend.client->calls.size(), callCount);
    QVERIFY(backend.breakpointAnswers.value(4).first);

    backend.client->failing.insert("disableBreakpoint");
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Update, 5, false));
    QVERIFY(!backend.breakpointAnswers.value(5).first);
}

void tst_uvsc::insertsDisabledBreakpoint()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Insert, 1, false));
    QCOMPARE(backend.client->calls.last(), "disableBreakpoint");
    QCOMPARE(enabledIn(backend.breakpointAnswers.value(1)), "n");
}

void tst_uvsc::removesBreakpoint()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Insert, 1, true));
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Remove, 2, true));
    QCOMPARE(backend.client->calls.last(), "deleteBreakpoint");
    QCOMPARE(backend.client->breakpointTickMark, 42u);
    QVERIFY(backend.breakpointAnswers.value(2).first);

    // It is gone, so there is nothing to change.
    backend.changeBreakpoint(lineBreakpoint(BreakpointOp::Update, 3, false));
    QVERIFY(!backend.breakpointAnswers.value(3).first);
}

void tst_uvsc::passesExpandedLocalsAndWatchers()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.activateFrame(2);

    RefreshRequest request;
    request.requestId = 11;
    request.kind = RefreshKind::Locals;
    request.expandedINames = {"local.a", "local.a.b", "watch.0", "return.x"};
    request.watchers = QJsonArray({QJsonObject({{"iname", "watch.0"}, {"exp", "x + 1"}})});
    backend.refresh(request);

    // Answering right away would trip the watch model.
    QVERIFY(!backend.refreshes.contains(11));
    QTRY_VERIFY(backend.refreshes.contains(11));

    QCOMPARE(backend.client->expandedLocals, QStringList({"local.a", "local.a.b"}));
    QCOMPARE(backend.client->expandedWatchers, QStringList({"watch.0"}));
    QCOMPARE(backend.client->watchers.size(), size_t(1));
    QCOMPARE(backend.client->watchers.front().first, "watch.0");
    QCOMPARE(backend.client->watchers.front().second, "x + 1");
    QCOMPARE(backend.client->localsTaskId, 7);
    QCOMPARE(backend.client->localsFrameId, 2);

    const GdbMi result = backend.refreshes.value(11);
    QCOMPARE(result["partial"].data(), "0");
    QCOMPARE(result["data"].childCount(), 2);
    QCOMPARE(result["data"].childAt(0)["iname"].data(), "local.a");
    QCOMPARE(result["data"].childAt(1)["iname"].data(), "watch.0");
}

void tst_uvsc::reportsRegisters()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.refresh({21, RefreshKind::Registers});
    const GdbMi data = backend.refreshes.value(21);
    QCOMPARE(data.childCount(), 1);
    const GdbMi reg = data.childAt(0);
    QCOMPARE(reg["name"].data(), "R0");
    QCOMPARE(reg["size"].data(), "4");
    QCOMPARE(reg["description"].data(), "Core");
    QCOMPARE(reg["value"].data(), "00001234");

    backend.setRegisterValue("R0", "0x55");
    QCOMPARE(backend.client->registerIndex, 3);
    QCOMPARE(backend.client->registerValue, "0x55");

    const int callCount = backend.client->calls.size();
    backend.setRegisterValue("R99", "0x55");
    QCOMPARE(backend.client->calls.size(), callCount);
}

void tst_uvsc::reportsPeripheralRegisters()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.client->memoryBase = 0x40000000;
    backend.client->memory = QByteArray("\x01\x00\x00\x00\x02\x01\x00\x00", 8);

    RefreshRequest request;
    request.requestId = 31;
    request.kind = RefreshKind::PeripheralRegisters;
    request.addresses = {0x40000000, 0x40000004};
    backend.refresh(request);

    QList<GdbMi> answers = backend.refreshes.values(31);
    QCOMPARE(answers.size(), 2);
    std::sort(answers.begin(), answers.end(), [](const GdbMi &a, const GdbMi &b) {
        return a["address"].data().toULongLong() < b["address"].data().toULongLong();
    });
    QCOMPARE(answers.at(0)["address"].data(), QString::number(0x40000000));
    QCOMPARE(answers.at(0)["value"].data(), "1");
    QCOMPARE(answers.at(1)["address"].data(), QString::number(0x40000004));
    QCOMPARE(answers.at(1)["value"].data(), "258");

    backend.setPeripheralRegisterValue(0x40000008, 0x0102);
    QCOMPARE(backend.client->changedAddress, quint64(0x40000008));
    QCOMPARE(backend.client->changedData, QByteArray("\x02\x01\x00\x00", 4));
}

void tst_uvsc::reportsThreadsAndStack()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    emit backend.client->locationUpdated(0x8000210);

    backend.refresh({41, RefreshKind::Threads});
    const GdbMi threads = backend.refreshes.value(41);
    QCOMPARE(threads["threads"].childCount(), 1);
    QCOMPARE(threads["current-thread-id"].data(), "7");

    backend.refresh({42, RefreshKind::FullStack});
    QCOMPARE(backend.client->stackTaskId, 7u);
    QCOMPARE(backend.client->stackAddress, quint64(0x8000210));
    QCOMPARE(backend.refreshes.value(42)["stack"]["frames"].childCount(), 2);

    backend.refresh({43, RefreshKind::FullBacktrace});
    const QString text = backend.refreshes.value(43).data();
    QCOMPARE(text, "#1  0x8000201 in main at C:/src/main.c:21\n"
                   "#2  0x8000202 in foo at C:/src/main.c:22\n");

    // The stack stays as it was when uVision has none to give.
    backend.client->failing.insert("fetchStackFrames");
    backend.refresh({44, RefreshKind::FullStack});
    QVERIFY(!backend.refreshes.contains(44));
}

void tst_uvsc::reportsModules()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.refresh({51, RefreshKind::Modules});
    const GdbMi data = backend.refreshes.value(51);
    QCOMPARE(data.childCount(), 1);
    QCOMPARE(data.childAt(0)["modulepath"].data(),
             startData().inferiorExecutable.toFSPathString());
    QCOMPARE(data.childAt(0)["modulename"].data(), "<executable>");
}

void tst_uvsc::parsesDisassembly()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.client->disassembly = "    25:     struct foo foo = {0}; \n"
                                  "0x08000210 9101      STR      r1,[sp,#0x04]\n"
                                  "some comment\n";
    backend.fetchDisassembly(0x8000210);
    QCOMPARE(backend.client->disassembledAddress, quint64(0x8000210));

    const QList<DisassemblerLine> lines = backend.disassembled.data();
    QCOMPARE(lines.size(), 3);
    QCOMPARE(lines.at(0).lineNumber, 25);
    QCOMPARE(lines.at(0).data, "     struct foo foo = {0}; ");
    QCOMPARE(lines.at(1).address, quint64(0x8000210));
    QCOMPARE(lines.at(1).bytes, "9101");
    QCOMPARE(lines.at(1).data, "STR      r1,[sp,#0x04]");
    QVERIFY(lines.at(2).isComment());
    QCOMPARE(lines.at(2).data, "some comment");
}

void tst_uvsc::readsMemory()
{
    DebuggerBackend backend(startData());
    QVERIFY(backend.setUp());
    backend.client->memoryBase = 0x20000000;
    backend.client->memory = "abc";

    backend.accessMemory(MemoryOp::Fetch, 0x20000000, 8);
    QCOMPARE(backend.memoryReads.size(), 1);
    const MemoryReadResult shortRead = backend.memoryReads.at(0);
    QCOMPARE(shortRead.address, quint64(0x20000000));
    QCOMPARE(shortRead.data.left(3), QByteArray("abc"));
    QCOMPARE(shortRead.unreadable.size(), 1);
    QCOMPARE(shortRead.unreadable.at(0).offset, quint64(3));
    QCOMPARE(shortRead.unreadable.at(0).length, quint64(5));

    backend.client->failing.insert("fetchMemory");
    backend.accessMemory(MemoryOp::Fetch, 0x20000000, 8);
    QCOMPARE(backend.memoryReads.size(), 2);
    QCOMPARE(backend.memoryReads.at(1).unreadable.size(), 1);
    QCOMPARE(backend.memoryReads.at(1).unreadable.at(0).offset, quint64(0));
    QCOMPARE(backend.memoryReads.at(1).unreadable.at(0).length, quint64(8));

    backend.accessMemory(MemoryOp::Change, 0x20000004, 2, "xy");
    QCOMPARE(backend.client->changedAddress, quint64(0x20000004));
    QCOMPARE(backend.client->changedData, QByteArray("xy"));
}

QTEST_GUILESS_MAIN(tst_uvsc)

#include "tst_uvsc.moc"
