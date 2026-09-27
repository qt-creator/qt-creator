// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "breakpoint.h"
#include "debuggerengineinterface.h"
#include "debuggerprotocol.h"

#include "jdwp/jdwpclient.h"
#include "jdwp/jdwpimpl.h"

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>
#include <utils/shutdownguard.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTemporaryDir>
#include <QTest>
#include <QtEndian>

#include <limits>
#include <memory>

using namespace Debugger::Internal;
using namespace Utils;

// Starting a virtual machine takes a second or two, more on a loaded machine.
static constexpr int s_timeoutMs = 60000;

static const char s_mainClass[] = "org.qtproject.jdwptest.Inferior";

// Drives the backend the way the engine does. The interface keeps its calls
// private to the engine and to this class.
class DebuggerBackend : public QObject
{
public:
    explicit DebuggerBackend(const JdwpImplStartData &startData)
        : m_engine(new JdwpImpl(startData))
    {
        connect(m_engine.get(), &DebuggerEngineInterface::message, this,
                [this](const QString &text, int channel, int) {
            if (channel == Debugger::AppOutput)
                m_output += text;
            m_log.append(text);
        });
        connect(m_engine.get(), &DebuggerEngineInterface::inferiorEvent, this,
                [this](InferiorEvent event) {
            m_events.append(event);
            // The engine hands its breakpoints over while it hears this.
            if (event == InferiorEvent::EngineSetupOk) {
                if (m_initialThrow != 0)
                    insertThrowBreakpoint(m_initialThrow > 0);
                for (const auto &[file, line, condition, ignoreCount]
                     : std::as_const(m_initialBreakpoints)) {
                    insertBreakpoint(file, line, condition, ignoreCount);
                }
            }
        });
        connect(m_engine.get(), &DebuggerEngineInterface::locationChanged, this,
                [this](const FilePath &file, int line) {
            m_stoppedFile = file;
            m_stoppedLine = line;
        });
        connect(m_engine.get(), &DebuggerEngineInterface::inferiorDone, this,
                [this](const InferiorResultData &result) { m_results.append(result); });
        connect(m_engine.get(), &DebuggerEngineInterface::breakpointEvent, this,
                [this](quint64 requestId, BreakpointOp, bool ok, const GdbMi &data) {
            m_breakpointAnswers.insert(requestId, {ok, data});
        });
        connect(m_engine.get(), &DebuggerEngineInterface::breakpointModified, this,
                [this](const GdbMi &data) { m_modified.append(data); });
        connect(m_engine.get(), &DebuggerEngineInterface::refreshDataReceived, this,
                [this](quint64 requestId, RefreshKind, const GdbMi &data) {
            m_refreshes.insert(requestId, data);
        });
        connect(m_engine.get(), &DebuggerEngineInterface::threadEvent, this,
                [this](ThreadEvent event, const GdbMi &) {
            if (event == ThreadEvent::Created)
                ++m_threadsCreated;
        });
    }

    ~DebuggerBackend() override { m_engine->disconnect(); }

    void addInitialBreakpoint(const FilePath &file, int line, const QString &condition = {},
                              int ignoreCount = 0)
    {
        m_initialBreakpoints.append({file, line, condition, ignoreCount});
    }

    void addInitialThrowBreakpoint(bool enabled = true) { m_initialThrow = enabled ? 1 : -1; }

    void start() { m_engine->start(); }
    void execute(ExecutionCommand command) { m_engine->execute({command}); }
    void execute(const ExecutionRequest &request) { m_engine->execute(request); }
    void shutdownInferior(ShutdownMode mode) { m_engine->shutdownInferior(mode); }
    void shutdownEngine() { m_engine->shutdownEngine(); }
    void selectThread(const QString &id) { m_engine->selectThread(id); }
    void activateFrame(int index) { m_engine->activateFrame(index); }

    quint64 insertBreakpoint(const FilePath &file, int line, const QString &condition = {},
                             int ignoreCount = 0)
    {
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = m_nextRequestId++;
        request.params = BreakpointParameters(BreakpointByFileAndLine);
        request.params.fileName = file;
        request.params.textPosition = {line, -1};
        request.params.enabled = true;
        request.params.condition = condition;
        request.params.ignoreCount = ignoreCount;
        m_engine->changeBreakpoint(request);
        return request.requestId;
    }

    quint64 removeBreakpoint(const QString &number)
    {
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Remove;
        request.requestId = m_nextRequestId++;
        request.responseId = number;
        m_engine->changeBreakpoint(request);
        return request.requestId;
    }

    quint64 setBreakpointEnabled(const QString &number, const FilePath &file, int line,
                                 bool enabled, int ignoreCount = 0)
    {
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Update;
        request.requestId = m_nextRequestId++;
        request.responseId = number;
        request.params = BreakpointParameters(BreakpointByFileAndLine);
        request.params.fileName = file;
        request.params.textPosition = {line, -1};
        request.params.enabled = enabled;
        request.params.ignoreCount = ignoreCount;
        m_engine->changeBreakpoint(request);
        return request.requestId;
    }

    quint64 insertThrowBreakpoint(bool enabled = true, const QString &condition = {})
    {
        BreakpointChangeRequest request;
        request.op = BreakpointOp::Insert;
        request.requestId = m_nextRequestId++;
        request.params = BreakpointParameters(BreakpointAtThrow);
        request.params.enabled = enabled;
        request.params.condition = condition;
        m_engine->changeBreakpoint(request);
        return request.requestId;
    }

    quint64 refresh(RefreshKind kind, const QSet<QString> &expanded = {},
                    const QStringList &watchers = {}, bool allowInferiorCalls = true)
    {
        RefreshRequest request;
        request.requestId = m_nextRequestId++;
        request.kind = kind;
        request.expandedINames = expanded;
        request.allowInferiorCalls = allowInferiorCalls;
        // The model hands the expressions over hex-encoded, under an iname of
        // its own making.
        QJsonArray items;
        for (int i = 0; i < watchers.size(); ++i) {
            items.append(QJsonObject{
                {"iname", QString("watch.%1").arg(i)},
                {"exp", QString::fromLatin1(watchers.at(i).toUtf8().toHex())}});
        }
        request.watchers = items;
        m_engine->refresh(request);
        return request.requestId;
    }

    void assign(const QString &expression, const QString &value)
    {
        m_engine->assignValueInDebugger({}, expression, value);
    }

    bool answered(quint64 requestId) const { return m_breakpointAnswers.contains(requestId); }
    bool answeredOk(quint64 requestId) const { return m_breakpointAnswers.value(requestId).first; }
    GdbMi answer(quint64 requestId) const { return m_breakpointAnswers.value(requestId).second; }
    // The number the backend gave the breakpoint, which is how it is named later.
    QString numberOf(quint64 requestId) const
    {
        return answer(requestId).childAt(0)["number"].data();
    }

    bool refreshed(quint64 requestId) const { return m_refreshes.contains(requestId); }
    GdbMi refreshData(quint64 requestId) const { return m_refreshes.value(requestId); }

    qsizetype count(InferiorEvent event) const { return m_events.count(event); }
    bool contains(InferiorEvent event) const { return m_events.contains(event); }
    qsizetype stops() const
    {
        return count(InferiorEvent::StopOk) + count(InferiorEvent::SpontaneousStop);
    }

    void clearStoppedLocation() { m_stoppedFile = {}; m_stoppedLine = 0; }
    FilePath stoppedFile() const { return m_stoppedFile; }
    int stoppedLine() const { return m_stoppedLine; }
    const QList<InferiorResultData> &results() const { return m_results; }
    const QList<GdbMi> &modified() const { return m_modified; }
    QString output() const { return m_output; }
    bool logged(const QString &text) const { return logCount(text) > 0; }
    qsizetype logCount(const QString &text) const
    {
        return std::count_if(m_log.cbegin(), m_log.cend(), [&text](const QString &line) {
            return line.contains(text);
        });
    }
    QString log() const { return m_log.mid(qMax(0, m_log.size() - 60)).join('\n'); }
    int threadsCreated() const { return m_threadsCreated; }

private:
    std::unique_ptr<DebuggerEngineInterface> m_engine;
    class InitialBreakpoint
    {
    public:
        FilePath file;
        int line = 0;
        QString condition;
        int ignoreCount = 0;
    };
    QList<InitialBreakpoint> m_initialBreakpoints;
    // Nothing, one that is enabled, or one that is not.
    int m_initialThrow = 0;
    QList<InferiorEvent> m_events;
    QHash<quint64, QPair<bool, GdbMi>> m_breakpointAnswers;
    QHash<quint64, GdbMi> m_refreshes;
    QList<InferiorResultData> m_results;
    QList<GdbMi> m_modified;
    QStringList m_log;
    QString m_output;
    FilePath m_stoppedFile;
    int m_stoppedLine = 0;
    int m_threadsCreated = 0;
    quint64 m_nextRequestId = 1;
};

// An agent that answers what a session starts with, hangs up on the command it
// was told about instead of answering it, and says what else it was asked.
class FakeAgent : public QObject
{
public:
    explicit FakeAgent(quint8 commandSet = Jdwp::VirtualMachineSet,
                       quint8 command = Jdwp::VmDispose, int idSize = 8)
        : m_hangUpSet(commandSet)
        , m_hangUpCommand(command)
        , m_idSize(idSize)
    {
        m_server.listen(QHostAddress::LocalHost);
        connect(&m_server, &QTcpServer::newConnection, this, [this] {
            m_socket = m_server.nextPendingConnection();
            connect(m_socket, &QTcpSocket::readyRead, this, &FakeAgent::handleData);
        });
    }

    quint16 port() const { return m_server.serverPort(); }
    bool hungUp() const { return m_hungUp; }
    int resumes() const { return m_resumes; }
    // How much the agent had been asked to set up when it was first resumed,
    // which tells a resume that waited for the breakpoints from one that did
    // not.
    int requestsBeforeTheFirstResume() const { return m_requestsBeforeTheFirstResume; }

    // One event set holding the start of the virtual machine and a class that
    // was prepared, which suspends the machine once for both.
    QByteArray startAndClassPrepare() const
    {
        JdwpWriter body(sizes());
        body.writeByte(Jdwp::SuspendAll).writeInt(2);
        body.writeByte(Jdwp::VmStartEvent).writeInt(0).writeObjectId(1);
        body.writeByte(Jdwp::ClassPrepareEvent).writeInt(0).writeObjectId(1)
            .writeByte(Jdwp::ClassTag).writeReferenceTypeId(2)
            .writeString(QString("L%1;").arg(QString(s_mainClass).replace('.', '/')))
            .writeInt(7);
        return body.data();
    }

    // A suspension that comes with nothing a debugger can make sense of: an
    // event of an unknown kind says nothing about how long it is.
    QByteArray unreadableEvent() const
    {
        JdwpWriter body(sizes());
        body.writeByte(Jdwp::SuspendAll).writeInt(1).writeByte(200).writeInt(0);
        return body.data();
    }

    void sendStartAndClassPrepare() { sendEvents(startAndClassPrepare()); }

    // What the machine says as soon as hands have been shaken, which is before
    // it has answered anything it is asked.
    void sendOnHandshake(const QByteArray &events) { m_onHandshake = events; }

private:
    JdwpIdSizes sizes() const
    {
        JdwpIdSizes sizes;
        sizes.fieldId = sizes.methodId = sizes.objectId = m_idSize;
        sizes.referenceTypeId = sizes.frameId = m_idSize;
        return sizes;
    }

    void sendEvents(const QByteArray &body)
    {
        QByteArray packet(11, Qt::Uninitialized);
        qToBigEndian<quint32>(quint32(11 + body.size()), packet.data());
        qToBigEndian<quint32>(quint32(0), packet.data() + 4);
        packet[8] = 0;
        packet[9] = char(Jdwp::EventSet);
        packet[10] = char(Jdwp::EventComposite);
        m_socket->write(packet + body);
    }

    void handleData()
    {
        m_buffer += m_socket->readAll();
        if (!m_shookHands) {
            if (m_buffer.size() < 14)
                return;
            m_socket->write(m_buffer.left(14));
            m_buffer.remove(0, 14);
            m_shookHands = true;
            if (!m_onHandshake.isEmpty()) {
                sendEvents(m_onHandshake);
                m_onHandshake.clear();
            }
        }
        while (m_buffer.size() >= 11) {
            const quint32 length = qFromBigEndian<quint32>(m_buffer.constData());
            if (quint32(m_buffer.size()) < length)
                return;
            const quint32 id = qFromBigEndian<quint32>(m_buffer.constData() + 4);
            const quint8 commandSet = quint8(m_buffer.at(9));
            const quint8 command = quint8(m_buffer.at(10));
            m_buffer.remove(0, length);

            if (commandSet == Jdwp::EventRequestSet && command == Jdwp::EventRequestSetCommand)
                ++m_requests;
            if (commandSet == Jdwp::VirtualMachineSet && command == Jdwp::VmResume) {
                if (m_resumes == 0)
                    m_requestsBeforeTheFirstResume = m_requests;
                ++m_resumes;
            }
            if (commandSet == m_hangUpSet && command == m_hangUpCommand) {
                m_hungUp = true;
                m_socket->disconnectFromHost();
                return;
            }
            JdwpWriter body(sizes());
            if (commandSet == Jdwp::VirtualMachineSet && command == Jdwp::VmIdSizes) {
                for (int i = 0; i < 5; ++i)
                    body.writeInt(m_idSize);
            } else if (commandSet == Jdwp::VirtualMachineSet
                       && command == Jdwp::VmCapabilitiesNew) {
                for (int i = 0; i < 32; ++i)
                    body.writeBool(false);
            } else if (commandSet == Jdwp::EventRequestSet
                       && command == Jdwp::EventRequestSetCommand) {
                body.writeInt(qint32(id));
            }
            QByteArray reply(11, Qt::Uninitialized);
            qToBigEndian<quint32>(quint32(11 + body.data().size()), reply.data());
            qToBigEndian<quint32>(id, reply.data() + 4);
            reply[8] = char(0x80);
            reply[9] = 0;
            reply[10] = 0;
            m_socket->write(reply + body.data());
        }
    }

    QTcpServer m_server;
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    QByteArray m_onHandshake;
    bool m_shookHands = false;
    const quint8 m_hangUpSet;
    const quint8 m_hangUpCommand;
    const int m_idSize;
    bool m_hungUp = false;
    int m_resumes = 0;
    int m_requests = 0;
    int m_requestsBeforeTheFirstResume = 0;
};

static GdbMi childNamed(const GdbMi &list, const QString &name)
{
    for (const GdbMi &child : list) {
        if (child["name"].data() == name)
            return child;
    }
    return {};
}

static QStringList namesIn(const GdbMi &list)
{
    QStringList names;
    for (const GdbMi &child : list)
        names.append(child["name"].data());
    return names;
}

class tst_jdwp : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void clientShakesHandsAndAsksForTheVersion();
    void failsTheSetupWhenTheVirtualMachineCannotStart();
    void runsToTheEndAndReportsTheExitCode();
    void stopsInAClassThatIsNotLoadedYet();
    void stopsAtABreakpointSetWhileStopped();
    void movesABreakpointToTheNextLineWithCode();
    void stepsInOverAndOut();
    void reportsTheStack();
    void findsASourceThroughTheProjectFiles();
    void readsLocalsAndExpandsThem();
    void readsTheFieldsOfThis();
    void showsAnObjectByItsToString();
    void leavesAnObjectAloneWhenCallsAreNotAllowed();
    void givesUpOnACallThatDoesNotComeBack();
    void runsPastABreakpointInsideACallItMade();
    void keepsACallItGaveUpOnApartFromOneOnAnotherThread();
    void stopsWhereAnExceptionIsThrown();
    void stopsAtAnExceptionThrownInsideTheRuntime();
    void runsPastAnExceptionTheRuntimeCatchesItself();
    void runsPastAnExceptionBreakpointThatIsDisabled();
    void findsThePackageOfASourceWithAndWithoutASemicolon();
    void takesABreakpointInEveryVirtualMachineSource();
    void parsesExpressions();
    void readsWatchers();
    void readsAWatcherOfThis();
    void readsAndWritesStringsTheMachineWay();
    void writesValues();
    void stopsOnlyWhereAConditionHolds();
    void ignoresTheFirstHits();
    void ignoresHitsFromWhenTheCountIsSet();
    void stopsWhenAConditionCannotBeWorkedOut();
    void listsTheThreads();
    void doesNotStopAtARemovedBreakpoint();
    void doesNotStopAtADisabledBreakpoint();
    void runsToALine();
    void interruptsARunningProgram();
    void attachesAndDetaches();
    void detachesFromALaunchedProgram();
    void detachesFromAnAgentThatHangsUp();
    void answersTheCommandsThatWereStillOut();
    void givesBackOneSuspensionPerEventSet();
    void readsAnEventThatCameBeforeTheHandleSizes();
    void holdsASuspendedMachineUntilItIsSetUp();

private:
    std::unique_ptr<DebuggerBackend> launch(const QStringList &arguments = {});
    int lineOf(const FilePath &file, const QString &marker) const;
    // A virtual machine that waits for a debugger at a port it chose itself.
    void startListeningVm(Process &process, QString &output, quint16 &port) const;

    FilePath m_java;
    FilePath m_sourceRoot;
    FilePath m_inferiorSource;
    FilePath m_helperSource;
    std::unique_ptr<QTemporaryDir> m_classesDir;
};

void tst_jdwp::initTestCase()
{
    // A JDK, rather than a runtime only, to compile the inferior with.
    FilePath javaHome = FilePath::fromUserInput(qtcEnvironmentVariable("QTC_JAVA_HOME_FOR_TEST"));
    if (javaHome.isEmpty())
        javaHome = FilePath::fromUserInput(qtcEnvironmentVariable("JAVA_HOME"));
    FilePath javac = javaHome.isEmpty() ? FilePath::fromString("javac").searchInPath()
                                        : (javaHome / "bin/javac").withExecutableSuffix();
    if (!javac.isExecutableFile())
        QSKIP("No JDK found: set QTC_JAVA_HOME_FOR_TEST or JAVA_HOME, or put javac in PATH.");
    m_java = javac.parentDir().pathAppended("java").withExecutableSuffix();
    QVERIFY2(m_java.isExecutableFile(), qPrintable(m_java.toUserOutput()));

    m_sourceRoot = FilePath::fromUserInput(JDWP_TEST_SOURCE_DIR);
    m_inferiorSource = m_sourceRoot / "org/qtproject/jdwptest/Inferior.java";
    m_helperSource = m_sourceRoot / "org/qtproject/jdwptest/Helper.java";

    m_classesDir = std::make_unique<QTemporaryDir>();
    QVERIFY(m_classesDir->isValid());
    Process compile;
    compile.setCommand({javac, {"-g", "-d", m_classesDir->path(),
                                m_inferiorSource.nativePath(), m_helperSource.nativePath()}});
    compile.runBlocking(std::chrono::seconds(120));
    QVERIFY2(compile.result() == ProcessResult::FinishedWithSuccess,
             qPrintable(compile.verboseExitMessage() + '\n' + compile.allOutput()));
}

int tst_jdwp::lineOf(const FilePath &file, const QString &marker) const
{
    const QStringList lines = QString::fromUtf8(file.fileContents().value_or(QByteArray()))
                                  .split('\n');
    for (qsizetype i = 0; i < lines.size(); ++i) {
        if (lines.at(i).trimmed().endsWith("// " + marker))
            return int(i + 1);
    }
    return 0;
}

std::unique_ptr<DebuggerBackend> tst_jdwp::launch(const QStringList &arguments)
{
    ProcessRunData runData;
    runData.command = CommandLine(m_java, {"-cp", m_classesDir->path(), s_mainClass});
    runData.command.addArgs(arguments);
    runData.workingDirectory = FilePath::fromUserInput(m_classesDir->path());
    runData.environment = Environment::systemEnvironment();
    return std::make_unique<DebuggerBackend>(
        JdwpImplStartData{.inferiorStartData = runData, .sourceSearchPaths = {m_sourceRoot}});
}

void tst_jdwp::startListeningVm(Process &process, QString &output, quint16 &port) const
{
    QObject::connect(&process, &Process::readyReadStandardOutput, &process,
                     [&process, &output] { output += process.readAllStandardOutput(); });
    process.setCommand({m_java, {"-agentlib:jdwp=transport=dt_socket,server=y,suspend=y,"
                                 "address=127.0.0.1:0",
                                 "-cp", m_classesDir->path(), s_mainClass}});
    process.start();
    static const QRegularExpression listening("Listening for transport dt_socket at address: "
                                              "(\\d+)");
    QRegularExpressionMatch match;
    const auto announced = [&] {
        match = listening.match(output);
        return match.hasMatch();
    };
    QTRY_VERIFY_WITH_TIMEOUT(announced() || process.state() == ProcessState::NotRunning,
                             s_timeoutMs);
    QVERIFY2(match.hasMatch(), qPrintable("The virtual machine announced no port: " + output));
    port = quint16(match.captured(1).toUInt());
}

void tst_jdwp::clientShakesHandsAndAsksForTheVersion()
{
    Process vm;
    QString output;
    quint16 port = 0;
    startListeningVm(vm, output, port);
    if (QTest::currentTestFailed())
        return;

    JdwpClient client;
    bool connected = false;
    QString failure;
    connect(&client, &JdwpClient::connected, this, [&connected] { connected = true; });
    connect(&client, &JdwpClient::disconnected, this, [&failure](const QString &reason) {
        failure = reason;
    });
    client.connectToHost("127.0.0.1", port);
    QTRY_VERIFY_WITH_TIMEOUT(connected || !failure.isEmpty(), s_timeoutMs);
    QVERIFY2(connected, qPrintable(failure));
    QVERIFY(client.idSizes().objectId > 0);

    std::optional<JdwpReply> reply;
    client.send(Jdwp::VirtualMachineSet, Jdwp::VmVersion, {},
                [&reply](const JdwpReply &answer) { reply = answer; });
    QTRY_VERIFY_WITH_TIMEOUT(reply.has_value(), s_timeoutMs);
    QVERIFY(reply->ok());
    JdwpReader reader = client.reader(reply->data);
    const QString description = reader.readString();
    const qint32 major = reader.readInt();
    reader.readInt(); // minor
    reader.readString(); // vm version
    const QString vmName = reader.readString();
    QVERIFY(reader.ok());
    QVERIFY2(major >= 1, qPrintable(description));
    QVERIFY(!vmName.isEmpty());

    client.close();
    vm.kill();
}

void tst_jdwp::failsTheSetupWhenTheVirtualMachineCannotStart()
{
    ProcessRunData runData;
    runData.command = CommandLine(m_sourceRoot / "no-such-java", {s_mainClass});
    DebuggerBackend backend(JdwpImplStartData{.inferiorStartData = runData});
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.contains(InferiorEvent::EngineSetupFailed), s_timeoutMs);
    QVERIFY(!backend.contains(InferiorEvent::EngineSetupOk));
}

void tst_jdwp::runsToTheEndAndReportsTheExitCode()
{
    const auto backend = launch();
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QVERIFY(backend->contains(InferiorEvent::EngineSetupOk));
    QVERIFY(backend->contains(InferiorEvent::RunAndInferiorRunOk));
    QCOMPARE(backend->results().first().exitCode, 3);
    QCOMPARE(backend->stops(), qsizetype(0));
    QTRY_VERIFY_WITH_TIMEOUT(backend->output().contains("result 49 6 12 hello 3"), s_timeoutMs);
}

void tst_jdwp::stopsInAClassThatIsNotLoadedYet()
{
    const int line = lineOf(m_helperSource, "nested-body");
    QVERIFY(line > 0);
    const auto backend = launch();
    backend->addInitialBreakpoint(m_helperSource, line);
    backend->start();

    // Nothing of the class is there before main() runs, so the breakpoint is
    // taken, but not set anywhere yet. The field says nothing: what it says is
    // read as a location, and this one has its own.
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(1), s_timeoutMs);
    QVERIFY(backend->answeredOk(1));
    const GdbMi taken = backend->answer(1).childAt(0);
    QVERIFY(taken["pending"].isValid());
    QVERIFY(taken["pending"].data().isEmpty());

    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    QCOMPARE(backend->stoppedFile(), m_helperSource);
    QCOMPARE(backend->stoppedLine(), line);
    QVERIFY(backend->contains(InferiorEvent::SpontaneousStop));

    // The class the breakpoint was waiting for has loaded by now.
    QVERIFY(!backend->modified().isEmpty());
    const GdbMi resolved = backend->modified().last().childAt(0);
    QCOMPARE(resolved["number"].data(), backend->numberOf(1));
    // The model marks by the field being there rather than by what it says,
    // so one that is in says nothing.
    QVERIFY(!resolved["pending"].isValid());
    QCOMPARE(resolved["line"].toInt(), line);

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->results().first().exitCode, 3);
    QVERIFY(backend->contains(InferiorEvent::RunOk));
}

void tst_jdwp::stopsAtABreakpointSetWhileStopped()
{
    const int startLine = lineOf(m_inferiorSource, "main-start");
    const int squareLine = lineOf(m_inferiorSource, "square-body");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, startLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), startLine);

    // The class is loaded now, so this one is set right away.
    const quint64 request = backend->insertBreakpoint(m_inferiorSource, squareLine);
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(request), s_timeoutMs);
    QVERIFY(backend->answeredOk(request));
    QVERIFY(!backend->answer(request).childAt(0)["pending"].isValid());

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(2));
    QCOMPARE(backend->stoppedLine(), squareLine);
    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::movesABreakpointToTheNextLineWithCode()
{
    const int commentLine = lineOf(m_inferiorSource, "no-code");
    const int codeLine = lineOf(m_inferiorSource, "after-no-code");
    QVERIFY(commentLine > 0 && codeLine > commentLine);
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, commentLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), codeLine);
    QVERIFY(!backend->modified().isEmpty());
    QCOMPARE(backend->modified().last().childAt(0)["line"].toInt(), codeLine);
    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::stepsInOverAndOut()
{
    const int callLine = lineOf(m_inferiorSource, "call-square");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, callLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), callLine);

    backend->execute(ExecutionCommand::StepIn);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), lineOf(m_inferiorSource, "square-body"));

    backend->execute(ExecutionCommand::StepOver);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 3 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), lineOf(m_inferiorSource, "square-return"));

    // Back in the caller, which has yet to store what the call returned.
    backend->execute(ExecutionCommand::StepOut);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 4 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), callLine);

    backend->execute(ExecutionCommand::StepOver);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 5 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), lineOf(m_inferiorSource, "after-square"));

    // Every step was announced and began, in the order the engine wants.
    QCOMPARE(backend->count(InferiorEvent::RunRequested), qsizetype(4));
    QCOMPARE(backend->count(InferiorEvent::RunOk), qsizetype(4));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    // A step that is over does not stop anything later.
    QCOMPARE(backend->stops(), qsizetype(5));
}

void tst_jdwp::reportsTheStack()
{
    const int squareLine = lineOf(m_inferiorSource, "square-body");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, squareLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::FullStack);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi frames = backend->refreshData(request)["stack"]["frames"];
    QCOMPARE(int(frames.childCount()), 2);
    QCOMPARE(frames.childAt(0)["function"].data(), QString("%1.square").arg(s_mainClass));
    QCOMPARE(frames.childAt(0)["line"].toInt(), squareLine);
    QCOMPARE(FilePath::fromUserInput(frames.childAt(0)["file"].data()), m_inferiorSource);
    QCOMPARE(frames.childAt(1)["function"].data(), QString("%1.main").arg(s_mainClass));
    QCOMPARE(frames.childAt(1)["line"].toInt(), lineOf(m_inferiorSource, "call-square"));
    QCOMPARE(frames.childAt(1)["level"].toInt(), 1);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::findsASourceThroughTheProjectFiles()
{
    ProcessRunData runData;
    runData.command = CommandLine(m_java, {"-cp", m_classesDir->path(), s_mainClass});
    // Holds the classes, but no sources to find there.
    runData.workingDirectory = FilePath::fromUserInput(m_classesDir->path());
    runData.environment = Environment::systemEnvironment();
    DebuggerBackend backend(JdwpImplStartData{.inferiorStartData = runData,
                                              .sourceFiles = {m_helperSource, m_inferiorSource}});
    backend.addInitialBreakpoint(m_helperSource, lineOf(m_helperSource, "nested-body"));
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.stops() == 1 || !backend.results().isEmpty(), s_timeoutMs);

    // The caller's file had no breakpoint to name it, so only the project knows it.
    const quint64 request = backend.refresh(RefreshKind::FullStack);
    QTRY_VERIFY_WITH_TIMEOUT(backend.refreshed(request), s_timeoutMs);
    const GdbMi caller = backend.refreshData(request)["stack"]["frames"].childAt(1);
    QCOMPARE(caller["function"].data(), QString("%1.main").arg(s_mainClass));
    QCOMPARE(FilePath::fromUserInput(caller["file"].data()), m_inferiorSource);

    backend.shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend.contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::readsLocalsAndExpandsThem()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "call-nested"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 collapsed = backend->refresh(RefreshKind::Locals);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(collapsed), s_timeoutMs);
    const GdbMi locals = backend->refreshData(collapsed)["data"];
    // A static method has no this, and the arguments come first.
    QCOMPARE(namesIn(locals), QStringList({"args", "number", "text", "values", "point",
                                           "squared", "total", "nested"}));
    QCOMPARE(childNamed(locals, "number")["value"].data(), QString("7"));
    QCOMPARE(childNamed(locals, "number")["type"].data(), QString("int"));
    QCOMPARE(childNamed(locals, "text")["value"].data(), QString("\"hello\""));
    QCOMPARE(childNamed(locals, "squared")["value"].data(), QString("49"));
    QCOMPARE(childNamed(locals, "values")["type"].data(), QString("int[]"));
    QCOMPARE(childNamed(locals, "values")["numchild"].data(), QString("1"));
    QCOMPARE(childNamed(locals, "args")["numchild"].data(), QString("0"));
    QCOMPARE(childNamed(locals, "point")["type"].data(),
             QString("%1$Point").arg(s_mainClass));
    QCOMPARE(childNamed(locals, "point")["numchild"].data(), QString("1"));
    // Collapsed, nothing below is fetched.
    QVERIFY(!childNamed(locals, "values")["children"].isValid());

    const quint64 expanded = backend->refresh(RefreshKind::Locals,
                                              {"local.values", "local.point"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(expanded), s_timeoutMs);
    const GdbMi expandedLocals = backend->refreshData(expanded)["data"];
    const GdbMi values = childNamed(expandedLocals, "values")["children"];
    QCOMPARE(namesIn(values), QStringList({"[0]", "[1]", "[2]"}));
    QCOMPARE(values.childAt(2)["value"].data(), QString("3"));
    QCOMPARE(values.childAt(2)["iname"].data(), QString("local.values.2"));
    const GdbMi point = childNamed(expandedLocals, "point")["children"];
    QCOMPARE(namesIn(point), QStringList({"x", "y"}));
    QCOMPARE(childNamed(point, "y")["value"].data(), QString("4"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::readsTheFieldsOfThis()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_helperSource, lineOf(m_helperSource, "nested-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Locals, {"local.this"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi locals = backend->refreshData(request)["data"];
    QCOMPARE(namesIn(locals).value(0), QString("this"));
    // javac lists it among the variables, too.
    QCOMPARE(namesIn(locals).count("this"), qsizetype(1));
    const GdbMi self = childNamed(locals, "this");
    QCOMPARE(self["type"].data(), QString("org.qtproject.jdwptest.Helper$Nested"));
    QCOMPARE(childNamed(self["children"], "value")["value"].data(), QString("6"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::showsAnObjectByItsToString()
{
    const auto backend = launch({"show"});
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "show-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));

    const quint64 request = backend->refresh(RefreshKind::Locals);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi locals = backend->refreshData(request)["data"];
    // A class that overrides toString() says what it is, and one call waits
    // for the other: each one runs the thread that both were read from.
    QCOMPARE(childNamed(locals, "located")["value"].data(), QString("(3, 4)"));
    QCOMPARE(childNamed(locals, "other")["value"].data(), QString("(5, 6)"));
    // One that does not keeps the identity it reads as, since what
    // java.lang.Object would answer says no more than that.
    const QString plain = childNamed(locals, "plain")["value"].data();
    QVERIFY2(plain.startsWith('@'), qPrintable(plain));
    // A line break in what one answers would break out of the cell it is
    // shown in, so it reads the way a string value does.
    QCOMPARE(childNamed(locals, "wordy")["value"].data(), QString("one\\ttwo\\nthree"));
    // It is still an object, with its fields below it.
    QCOMPARE(childNamed(locals, "located")["numchild"].data(), QString("1"));
    QCOMPARE(childNamed(locals, "located")["type"].data(),
             QString("%1$Point").arg(s_mainClass));

    const quint64 expanded = backend->refresh(RefreshKind::Locals, {"local.located"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(expanded), s_timeoutMs);
    const GdbMi fields = childNamed(backend->refreshData(expanded)["data"], "located")["children"];
    QCOMPARE(namesIn(fields), QStringList({"x", "y"}));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::leavesAnObjectAloneWhenCallsAreNotAllowed()
{
    const auto backend = launch({"show"});
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "show-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Locals, {}, {}, false);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi locals = backend->refreshData(request)["data"];
    // The one thing that would make the program run is not done here.
    const QString located = childNamed(locals, "located")["value"].data();
    QVERIFY2(located.startsWith('@'), qPrintable(located));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::givesUpOnACallThatDoesNotComeBack()
{
    const auto backend = launch({"slow"});
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "slow-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Locals);
    // The views hear back rather than waiting for the rest of the session.
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi locals = backend->refreshData(request)["data"];
    QVERIFY(childNamed(locals, "first")["value"].data().startsWith('@'));
    QVERIFY(childNamed(locals, "second")["value"].data().startsWith('@'));
    // And the second one is not even asked: the thread is off running the call
    // that never came back, and the machine takes no other while it does.
    QCOMPARE(backend->logCount("ObjectReference.InvokeMethod"), qsizetype(1));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

// A breakpoint the user set in a toString() is hit by the call the views make
// to show a value. Stopping there would leave the program standing in a call
// nobody asked for, and the call would never come back either: the thread it
// needs is the one the stop suspends.
void tst_jdwp::runsPastABreakpointInsideACallItMade()
{
    const auto backend = launch({"show"});
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "show-body"));
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "tostring-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Locals);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    // The call runs past the breakpoint and comes back with what it answers,
    // rather than being given up on at its timeout.
    const GdbMi locals = backend->refreshData(request)["data"];
    QCOMPARE(childNamed(locals, "located")["value"].data(), QString("(3, 4)"));
    QCOMPARE(childNamed(locals, "other")["value"].data(), QString("(5, 6)"));
    QVERIFY(backend->logged("stopped inside a call made to show a value"));
    // Said once for the call it happened in, however many times it was hit.
    QCOMPARE(backend->logCount("stopped inside a call made to show a value"), qsizetype(2));
    QCOMPARE(backend->stops(), qsizetype(1));

    // And nothing of it is left over to stop at the next time the program is
    // let go: a stop that was passed over is gone, not put by.
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

// A call given up on is still out: the thread it runs on has it, and every
// stop that thread runs into is still the call's rather than the user's. A
// call made on another thread is answered while it goes on, and answering
// that one says nothing about the one that was given up on.
void tst_jdwp::keepsACallItGaveUpOnApartFromOneOnAnotherThread()
{
    const auto backend = launch({"looping"});
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "looping-stop"));
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "looping-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    QCOMPARE(backend->stoppedLine(), lineOf(m_inferiorSource, "looping-stop"));

    // The call runs into the breakpoint in it, which is run past, and never
    // comes back: the views are let go of it at its timeout, and the object
    // keeps the identity it reads as.
    const quint64 locals = backend->refresh(RefreshKind::Locals);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(locals), s_timeoutMs);
    QVERIFY(backend->logged("stopped inside a call made to show a value"));
    const QString looping = childNamed(backend->refreshData(locals)["data"], "looping")["value"]
                                .data();
    QVERIFY2(looping.startsWith('@'), qPrintable(looping));
    QCOMPARE(backend->stops(), qsizetype(1));

    // Another thread is selected, and what it is standing in has an object of
    // its own to show. The call for it is made and answered - the machine
    // refuses it, since only a thread an event suspended takes one - while
    // the first call is still out.
    const quint64 threads = backend->refresh(RefreshKind::Threads);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(threads), s_timeoutMs);
    const GdbMi worker = childNamed(backend->refreshData(threads)["threads"], "worker");
    QVERIFY2(worker.isValid(),
             qPrintable(namesIn(backend->refreshData(threads)["threads"]).join(", ")));
    backend->selectThread(worker["id"].data());
    const quint64 onWorker = backend->refresh(RefreshKind::Locals);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(onWorker) || backend->stops() > 1, s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    QCOMPARE(childNamed(backend->refreshData(onWorker)["data"], "seen")["type"].data(),
             QString("%1$Point").arg(s_mainClass));
    QCOMPARE(backend->logCount("ObjectReference.InvokeMethod"), qsizetype(2));

    // And that answer is not the answer to the call that was given up on:
    // what that one keeps running into is still run past, rather than leaving
    // the program standing in a call nobody asked for.
    const qsizetype resumes = backend->logCount("VirtualMachine.Resume");
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() > 1
                                 || backend->logCount("VirtualMachine.Resume") > resumes + 1,
                             s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::stopsWhereAnExceptionIsThrown()
{
    const int caughtLine = lineOf(m_inferiorSource, "throw-caught");
    const int looseLine = lineOf(m_inferiorSource, "throw-uncaught");
    QVERIFY(caughtLine > 0 && looseLine > 0);
    const auto backend = launch({"throw"});
    backend->addInitialThrowBreakpoint();
    backend->start();

    // Nothing has to be loaded for it, so it is in as soon as it is taken,
    // and it stands at no line of any file.
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(1), s_timeoutMs);
    QVERIFY(backend->answeredOk(1));
    const GdbMi taken = backend->answer(1).childAt(0);
    QVERIFY(!taken["pending"].isValid());
    QVERIFY(!taken["file"].isValid());
    QVERIFY(!taken["line"].isValid());

    // Loading a class throws exceptions the runtime catches itself, so the
    // first one this stops at has to be the one the program throws.
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    QCOMPARE(backend->stoppedFile(), m_inferiorSource);
    QCOMPARE(backend->stoppedLine(), caughtLine);
    QTRY_VERIFY_WITH_TIMEOUT(backend->logged("java.lang.IllegalStateException"), s_timeoutMs);
    QVERIFY(backend->logged("is caught further up"));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(),
                             s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(2));
    QCOMPARE(backend->stoppedLine(), looseLine);
    QTRY_VERIFY_WITH_TIMEOUT(backend->logged("java.lang.IllegalArgumentException"), s_timeoutMs);
    QVERIFY(backend->logged("nothing catches it"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

// Most of what a debugger is wanted for is thrown by the runtime, not by the
// program: a number that will not parse, an index outside a list. Filtering by
// where the throw happens would leave exactly those unanswered.
void tst_jdwp::stopsAtAnExceptionThrownInsideTheRuntime()
{
    const auto backend = launch({"parse"});
    backend->addInitialThrowBreakpoint();
    backend->start();

    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    QTRY_VERIFY_WITH_TIMEOUT(backend->logged("java.lang.NumberFormatException"), s_timeoutMs);
    QVERIFY(backend->logged("nothing catches it"));
    // There is no source of java.lang.Integer to show, so the stop moves the
    // editor nowhere.
    QVERIFY(backend->stoppedFile().isEmpty());

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

// The other half of that: one the runtime throws and catches within itself is
// its own business, and a breakpoint at a throw is not set for it. Loading a
// class, looking up a character set and reaching for something by reflection
// all throw them, and stopping at each would make the breakpoint useless.
void tst_jdwp::runsPastAnExceptionTheRuntimeCatchesItself()
{
    const int looseLine = lineOf(m_inferiorSource, "throw-uncaught");
    QVERIFY(looseLine > 0);
    const auto backend = launch({"swallow"});
    backend->addInitialThrowBreakpoint();
    backend->start();

    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
    // The NumberFormatException that java.lang.Integer threw and caught on the
    // way here is not what this stops at; the program's own throw is.
    QCOMPARE(backend->stoppedFile(), m_inferiorSource);
    QCOMPARE(backend->stoppedLine(), looseLine);
    QTRY_VERIFY_WITH_TIMEOUT(backend->logged("java.lang.IllegalArgumentException"), s_timeoutMs);
    QVERIFY(!backend->logged("java.lang.NumberFormatException"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::runsPastAnExceptionBreakpointThatIsDisabled()
{
    const auto backend = launch({"throw"});
    backend->addInitialThrowBreakpoint(false);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(1), s_timeoutMs);
    QVERIFY(backend->answeredOk(1));

    // The exception that nothing catches is what ends the program.
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(0));
    QCOMPARE(backend->results().first().exitCode, 1);
}

void tst_jdwp::findsThePackageOfASourceWithAndWithoutASemicolon()
{
    QCOMPARE(packageDirectoryOf("package org.qtproject.jdwptest;\n\nclass A {}\n"),
             QString("org/qtproject/jdwptest"));
    // Kotlin writes the same statement, and ends it with the line.
    QCOMPARE(packageDirectoryOf("package org.qtproject.jdwptest\n\nclass A\n"),
             QString("org/qtproject/jdwptest"));
    QCOMPARE(packageDirectoryOf("// Copyright\n\npackage a.b;\n"), QString("a/b"));
    QCOMPARE(packageDirectoryOf("class A {}\n"), QString());
}

void tst_jdwp::takesABreakpointInEveryVirtualMachineSource()
{
    QVERIFY(isJvmSource(FilePath::fromString("/src/A.java")));
    QVERIFY(isJvmSource(FilePath::fromString("/src/A.kt")));
    QVERIFY(isJvmSource(FilePath::fromString("/src/A.kts")));
    QVERIFY(!isJvmSource(FilePath::fromString("/src/a.cpp")));

    // What this stops in, a native debugger must not claim.
    BreakpointParameters kotlin(BreakpointByFileAndLine);
    kotlin.fileName = FilePath::fromString("/src/A.kt");
    kotlin.textPosition = {3, -1};
    QVERIFY(!kotlin.isCppBreakpoint());

    JdwpImpl backend{JdwpImplStartData{}};
    const auto accepts = backend.setupData().acceptsBreakpoint;
    QVERIFY(bool(accepts));
    QVERIFY(accepts({.type = BreakpointByFileAndLine,
                     .fileName = FilePath::fromString("/src/A.kt")}));
    QVERIFY(accepts({.type = BreakpointByFileAndLine,
                     .fileName = FilePath::fromString("/src/A.java")}));
    QVERIFY(!accepts({.type = BreakpointByFileAndLine,
                      .fileName = FilePath::fromString("/src/a.cpp")}));
    QVERIFY(accepts({.type = BreakpointAtThrow}));
    // The virtual machine stops where an exception is thrown, and nowhere a
    // handler takes it.
    QVERIFY(!accepts({.type = BreakpointAtCatch}));
}

void tst_jdwp::parsesExpressions()
{
    using Kind = JdwpExpression::Kind;

    const Result<JdwpExpression> name = parseJdwpExpression("number");
    QVERIFY(name);
    QCOMPARE(name->kind, Kind::Name);
    QCOMPARE(name->text, QString("number"));

    const Result<JdwpExpression> field = parseJdwpExpression(" point . x ");
    QVERIFY(field);
    QCOMPARE(field->kind, Kind::Field);
    QCOMPARE(field->text, QString("x"));
    QCOMPARE(field->base->kind, Kind::Name);
    QCOMPARE(field->base->text, QString("point"));

    const Result<JdwpExpression> element = parseJdwpExpression("values[1].name");
    QVERIFY(element);
    QCOMPARE(element->kind, Kind::Field);
    QCOMPARE(element->base->kind, Kind::Index);
    QCOMPARE(element->base->base->text, QString("values"));
    QCOMPARE(element->base->index->kind, Kind::Int);
    QCOMPARE(element->base->index->integer, 1);

    QCOMPARE(parseJdwpExpression("this")->kind, Kind::This);
    QCOMPARE(parseJdwpExpression("null")->kind, Kind::Null);
    QCOMPARE(parseJdwpExpression("true")->integer, 1);
    QCOMPARE(parseJdwpExpression("0x1f")->integer, 31);
    // An int holds what an int holds: a hexadecimal literal may spell out the
    // bits of a negative number, a decimal one that does not fit is no int.
    QCOMPARE(parseJdwpExpression("0xffffffff")->integer, -1);
    QCOMPARE(parseJdwpExpression("2147483647")->integer, 2147483647);
    QCOMPARE(parseJdwpExpression("3000000000L")->integer, 3000000000LL);
    QCOMPARE(parseJdwpExpression("-7")->integer, -7);
    QCOMPARE(parseJdwpExpression("1_000_000")->integer, 1000000);
    QCOMPARE(parseJdwpExpression("12L")->kind, Kind::Long);
    QCOMPARE(parseJdwpExpression("1.5")->kind, Kind::Double);
    QCOMPARE(parseJdwpExpression("1.5")->number, 1.5);
    QCOMPARE(parseJdwpExpression("1.5f")->kind, Kind::Float);
    QCOMPARE(parseJdwpExpression("2e-3")->number, 0.002);
    QCOMPARE(parseJdwpExpression("'a'")->integer, qint64('a'));
    QCOMPARE(parseJdwpExpression("'\\n'")->integer, qint64('\n'));
    QCOMPARE(parseJdwpExpression("\"a\\tb\"")->text, QString("a\tb"));
    QCOMPARE(parseJdwpExpression("\"\\u0041\"")->text, QString("A"));

    // Operators bind the way Java binds them.
    const Result<JdwpExpression> sum = parseJdwpExpression("1 + 2 * 3");
    QVERIFY(sum);
    QCOMPARE(sum->kind, Kind::Binary);
    QCOMPARE(sum->op, JdwpExpression::Operator::Add);
    QCOMPARE(sum->right->op, JdwpExpression::Operator::Multiply);
    QCOMPARE(parseJdwpExpression("(1 + 2) * 3")->op, JdwpExpression::Operator::Multiply);
    // A comparison is looser than the arithmetic in it, and "&&" looser still.
    const Result<JdwpExpression> condition = parseJdwpExpression("a + 1 == b && c");
    QVERIFY(condition);
    QCOMPARE(condition->op, JdwpExpression::Operator::And);
    QCOMPARE(condition->base->op, JdwpExpression::Operator::Equal);
    QCOMPARE(condition->base->base->op, JdwpExpression::Operator::Add);
    // "<" is not the start of "<=", and "!" is not the start of "!=".
    QCOMPARE(parseJdwpExpression("a < b")->op, JdwpExpression::Operator::Less);
    QCOMPARE(parseJdwpExpression("a <= b")->op, JdwpExpression::Operator::LessEqual);
    QCOMPARE(parseJdwpExpression("a != b")->op, JdwpExpression::Operator::NotEqual);
    QCOMPARE(parseJdwpExpression("!a")->op, JdwpExpression::Operator::Not);
    // A minus in front of a number belongs to the number, in front of a name
    // it is an operator.
    QCOMPARE(parseJdwpExpression("-a")->op, JdwpExpression::Operator::Negate);
    QCOMPARE(parseJdwpExpression("-1")->kind, Kind::Int);

    // What it cannot read it says so about, rather than guessing at a meaning.
    for (const QString &bad : QStringList{"", "  ", "point.", "values[1", "(x", "1 +", "@x",
                                          "'ab'", "\"open", "0x", "a =", "a ? b : c",
                                          "3000000000", "0x1ffffffff"}) {
        const Result<JdwpExpression> result = parseJdwpExpression(bad);
        QVERIFY2(!result, qPrintable(QString("\"%1\" was read as an expression").arg(bad)));
        QVERIFY(!result.error().isEmpty());
    }
}

void tst_jdwp::readsWatchers()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "call-nested"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const QStringList expressions = {"number", "point.x", "values[1]", "values.length", "text",
                                     "calls", "point", "nope", "values[9]", "number + 1",
                                     "number > 3 && point.x == 3", "values[number - 6]",
                                     "text == null", "-number", "number / 0",
                                     "negative < 0", "negative / 2", "tiny + 0", "small * 1",
                                     "least / -1", "least % -1", "least - 1", "-least",
                                     "least * 2", "0.1f * 3 == 0.3f", "1.5f + 1",
                                     "text == \"hello\"", "\"hi\" != text", "point == \"x\""};
    const quint64 request = backend->refresh(RefreshKind::Locals, {"watch.6"}, expressions);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi data = backend->refreshData(request)["data"];

    // The locals are still there, and each watcher carries the expression it stands for.
    QVERIFY(namesIn(data).contains("squared"));
    const GdbMi number = childNamed(data, "number");
    QCOMPARE(number["iname"].data(), QString("watch.0"));
    QCOMPARE(number["value"].data(), QString("7"));
    QCOMPARE(number["type"].data(), QString("int"));
    QCOMPARE(QByteArray::fromHex(number["wname"].data().toLatin1()), QByteArray("number"));

    QCOMPARE(childNamed(data, "point.x")["value"].data(), QString("3"));
    QCOMPARE(childNamed(data, "values[1]")["value"].data(), QString("2"));
    QCOMPARE(childNamed(data, "values.length")["value"].data(), QString("3"));
    QCOMPARE(childNamed(data, "text")["value"].data(), QString("\"hello\""));
    // A static field of the class the frame is in, which square() counted up.
    QCOMPARE(childNamed(data, "calls")["value"].data(), QString("1"));

    // An object among the watchers expands the way a local does.
    const GdbMi point = childNamed(data, "point");
    QCOMPARE(point["type"].data(), QString("%1$Point").arg(s_mainClass));
    QCOMPARE(namesIn(point["children"]), QStringList({"x", "y"}));
    QCOMPARE(childNamed(point["children"], "y")["value"].data(), QString("4"));
    QCOMPARE(childNamed(point["children"], "y")["iname"].data(), QString("watch.6.y"));

    // Operators are worked out here rather than in the virtual machine.
    QCOMPARE(childNamed(data, "number + 1")["value"].data(), QString("8"));
    QCOMPARE(childNamed(data, "number + 1")["type"].data(), QString("int"));
    QCOMPARE(childNamed(data, "number > 3 && point.x == 3")["value"].data(), QString("true"));
    // An index is an expression of its own.
    QCOMPARE(childNamed(data, "values[number - 6]")["value"].data(), QString("2"));
    QCOMPARE(childNamed(data, "text == null")["value"].data(), QString("false"));
    QCOMPARE(childNamed(data, "-number")["value"].data(), QString("-7"));
    // A number that came in narrower than it is wide keeps its sign.
    QCOMPARE(childNamed(data, "negative < 0")["value"].data(), QString("true"));
    QCOMPARE(childNamed(data, "negative / 2")["value"].data(), QString("-2"));
    QCOMPARE(childNamed(data, "tiny + 0")["value"].data(), QString("-8"));
    QCOMPARE(childNamed(data, "small * 1")["value"].data(), QString("-300"));
    // The division that overflows answers what Java answers, rather than
    // trapping.
    QCOMPARE(childNamed(data, "least / -1")["value"].data(),
             QString::number(std::numeric_limits<qint64>::min()));
    QCOMPARE(childNamed(data, "least % -1")["value"].data(), QString("0"));
    // So do the ones that overflow the other way, which wrap in Java.
    QCOMPARE(childNamed(data, "least - 1")["value"].data(),
             QString::number(std::numeric_limits<qint64>::max()));
    QCOMPARE(childNamed(data, "-least")["value"].data(),
             QString::number(std::numeric_limits<qint64>::min()));
    QCOMPARE(childNamed(data, "least * 2")["value"].data(), QString("0"));
    // Float arithmetic stays float, which rounds differently from double.
    QCOMPARE(childNamed(data, "0.1f * 3 == 0.3f")["value"].data(), QString("true"));
    QCOMPARE(childNamed(data, "1.5f + 1")["type"].data(), QString("float"));
    // "hello" in the program is the same interned string as the literal here.
    QCOMPARE(childNamed(data, "text == \"hello\"")["value"].data(), QString("true"));
    QCOMPARE(childNamed(data, "\"hi\" != text")["value"].data(), QString("true"));
    QCOMPARE(childNamed(data, "point == \"x\"")["value"].data(), QString("false"));

    // What cannot be answered says why, in the place of the value.
    QVERIFY(childNamed(data, "nope")["value"].data().startsWith('<'));
    QVERIFY(childNamed(data, "values[9]")["value"].data().contains("outside"));
    QVERIFY(childNamed(data, "number / 0")["value"].data().contains("zero"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::readsAWatcherOfThis()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_helperSource, lineOf(m_helperSource, "nested-body"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Locals, {},
                                             {"this.value", "value", "doubled", "counted",
                                              "level", "this.level"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi data = backend->refreshData(request)["data"];
    QCOMPARE(childNamed(data, "this.value")["value"].data(), QString("6"));
    // A static field of a superclass is a name here as it is in Java.
    QCOMPARE(childNamed(data, "counted")["value"].data(), QString("17"));
    // A name the frame has no local for is looked for in the object it runs on.
    QCOMPARE(childNamed(data, "value")["value"].data(), QString("6"));
    // A field hiding one of the superclass is the one the name means.
    QCOMPARE(childNamed(data, "level")["value"].data(), QString("2"));
    QCOMPARE(childNamed(data, "this.level")["value"].data(), QString("2"));
    // A local is only a name from where it is in scope, which "doubled" is not
    // on the line that declares it.
    QVERIFY(childNamed(data, "doubled")["value"].data().startsWith('<'));

    // The next method has a local of the same name as the field, which wins.
    const quint64 inserted = backend->insertBreakpoint(m_helperSource,
                                                       lineOf(m_helperSource, "nested-shadow"));
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(inserted), s_timeoutMs);
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    const quint64 shadowed = backend->refresh(RefreshKind::Locals, {},
                                              {"value", "this.value", "this.level"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(shadowed), s_timeoutMs);
    const GdbMi shadowedData = backend->refreshData(shadowed)["data"];
    QCOMPARE(childNamed(shadowedData, "value")["value"].data(), QString("42"));
    QCOMPARE(childNamed(shadowedData, "this.value")["value"].data(), QString("6"));

    // Writing a field that hides one of the superclass writes the one it reads.
    backend->assign("this.level", "3");
    QTRY_COMPARE_WITH_TIMEOUT(
        childNamed(backend->refreshData(shadowed)["data"], "this.level")["value"].data(),
        QString("3"), s_timeoutMs);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

// The virtual machine holds a string as UTF-16 and the wire carries it as
// UTF-8, counted rather than terminated. One with a null character in it, or
// with a character outside the basic plane, only comes across whole if nothing
// on the way stops at the null or reads the four bytes as four characters.
void tst_jdwp::readsAndWritesStringsTheMachineWay()
{
    const QString awkward = QString("a") + QChar(QChar::Null) + 'b'
                            + QChar(char16_t(0xd83d)) + QChar(char16_t(0xde00)) + 'c';
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "call-nested"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    // The field is read from the machine, the literal is made there first and
    // then read back, so the two together cover both directions.
    const QString literal = "\"a\\u0000b\\ud83d\\ude00c\"";
    const quint64 request = backend->refresh(RefreshKind::Locals, {}, {"awkward", literal});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi data = backend->refreshData(request)["data"];
    QCOMPARE(childNamed(data, "awkward")["value"].data(), '"' + awkward + '"');
    QCOMPARE(childNamed(data, literal)["value"].data(), '"' + awkward + '"');
    QCOMPARE(childNamed(data, literal)["type"].data(), QString("java.lang.String"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::writesValues()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "call-nested"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const QStringList watchers = {"number", "values[1]", "point.x", "calls", "text",
                                  "values[0]", "squared"};
    const quint64 before = backend->refresh(RefreshKind::Locals, {"local.point"}, watchers);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(before), s_timeoutMs);
    const GdbMi locals = backend->refreshData(before)["data"];
    QCOMPARE(childNamed(locals, "number")["value"].data(), QString("7"));
    // A row of the Locals view is written through what it says it is, and
    // for a child that has to name the object it is a field of.
    const QString pointX = childNamed(childNamed(locals, "point")["children"], "x")["exp"].data();
    QCOMPARE(pointX, QString("(point).x"));

    // A local, an element of an array, a field of an object, a static field,
    // and a string, which the machine has to be asked to make.
    backend->assign("number", "11");
    backend->assign("values[1]", "20");
    backend->assign(pointX, "30");
    backend->assign("calls", "40");
    backend->assign("text", "\"other\"");
    // A write that the type refuses leaves what is there alone.
    backend->assign("number", "\"not a number\"");
    // So does an index that is not a whole number, rather than landing on
    // whatever its bits happen to say.
    backend->assign("values[1.5]", "77");
    // A value too large for the type it goes to stops at the end of its range,
    // which is what a Java cast does.
    backend->assign("squared", "1e30");

    // The backend answers the last request again once a write has gone
    // through, so the view it leaves behind is the one to read.
    const auto written = [&] {
        const GdbMi data = backend->refreshData(before)["data"];
        const auto valueOf = [&data](const char *name) {
            return childNamed(data, QString::fromLatin1(name))["value"].data();
        };
        return valueOf("number") == QString("11") && valueOf("values[1]") == QString("20")
               && valueOf("point.x") == QString("30") && valueOf("calls") == QString("40")
               && valueOf("text") == QString("\"other\"")
               && valueOf("squared") == QString::number(std::numeric_limits<qint32>::max());
    };
    QTRY_VERIFY_WITH_TIMEOUT(written(), s_timeoutMs);

    const quint64 after = backend->refresh(RefreshKind::Locals, {}, watchers);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(after), s_timeoutMs);
    const GdbMi data = backend->refreshData(after)["data"];
    QCOMPARE(childNamed(data, "number")["value"].data(), QString("11"));
    QCOMPARE(childNamed(data, "values[1]")["value"].data(), QString("20"));
    QCOMPARE(childNamed(data, "point.x")["value"].data(), QString("30"));
    QCOMPARE(childNamed(data, "calls")["value"].data(), QString("40"));
    QCOMPARE(childNamed(data, "text")["value"].data(), QString("\"other\""));
    QCOMPARE(childNamed(data, "values[0]")["value"].data(), QString("1"));
    QCOMPARE(childNamed(data, "squared")["value"].data(),
             QString::number(std::numeric_limits<qint32>::max()));
    // The locals show the same, since that is where the values are.
    QCOMPARE(childNamed(data, "number")["type"].data(), QString("int"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::stopsOnlyWhereAConditionHolds()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const auto backend = launch();
    // The loop adds 1, 2 and 3 in turn, so each of these holds on one round
    // only. Both are hit at the same place, and neither may hide the other.
    backend->addInitialBreakpoint(m_inferiorSource, loopLine, "total == 3");
    backend->addInitialBreakpoint(m_inferiorSource, loopLine, "total == 1");
    backend->start();
    const auto total = [&backend](quint64 request) {
        return childNamed(backend->refreshData(request)["data"], "total")["value"].data();
    };
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);
    const quint64 first = backend->refresh(RefreshKind::Locals, {}, {"total"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(first), s_timeoutMs);
    QCOMPARE(total(first), QString("1"));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);
    const quint64 second = backend->refresh(RefreshKind::Locals, {}, {"total"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(second), s_timeoutMs);
    QCOMPARE(total(second), QString("3"));

    // Nothing else in the program satisfies them, so the run goes to the end.
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(2));
    QCOMPARE(backend->results().first().exitCode, 3);
}

void tst_jdwp::ignoresTheFirstHits()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, loopLine, {}, 2);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);

    // The first two rounds were passed over, so this is the third.
    const quint64 request = backend->refresh(RefreshKind::Locals, {}, {"value"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    QCOMPARE(childNamed(backend->refreshData(request)["data"], "value")["value"].data(),
             QString("3"));
    // The ones passed over count as hits too, as they do in the views of gdb.
    QVERIFY(!backend->modified().isEmpty());
    QCOMPARE(backend->modified().last().childAt(0)["times"].data(), QString("3"));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(1));
}

void tst_jdwp::ignoresHitsFromWhenTheCountIsSet()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, loopLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    // One hit is behind it already, and the count is about the ones to come.
    const quint64 update = backend->setBreakpointEnabled(backend->numberOf(1), m_inferiorSource,
                                                         loopLine, true, 1);
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(update), s_timeoutMs);
    QVERIFY(backend->answeredOk(update));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    const quint64 request = backend->refresh(RefreshKind::Locals, {}, {"value"});
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    QCOMPARE(childNamed(backend->refreshData(request)["data"], "value")["value"].data(),
             QString("3"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::stopsWhenAConditionCannotBeWorkedOut()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, loopLine, "nothingHere == 1");
    backend->start();
    // A condition that cannot be worked out stops, and says why.
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);
    QVERIFY(backend->log().contains("condition of breakpoint"));

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::listsTheThreads()
{
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, lineOf(m_inferiorSource, "main-start"));
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    const quint64 request = backend->refresh(RefreshKind::Threads);
    QTRY_VERIFY_WITH_TIMEOUT(backend->refreshed(request), s_timeoutMs);
    const GdbMi data = backend->refreshData(request);
    const GdbMi main = childNamed(data["threads"], "main");
    QVERIFY2(main.isValid(), qPrintable(namesIn(data["threads"]).join(", ")));
    QCOMPARE(main["state"].data(), QString("stopped"));
    QCOMPARE(data["current-thread-id"].data(), main["id"].data());
    // The machine reports a thread as it starts, over the connection the stop
    // came in on, so by now those events have been in for a while. After the
    // program has ended there is no such order to rely on, which is why this
    // is asked here and not of a run that goes to the end.
    QVERIFY(backend->threadsCreated() > 0);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::doesNotStopAtARemovedBreakpoint()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const int nestedLine = lineOf(m_helperSource, "nested-body");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, loopLine);
    backend->addInitialBreakpoint(m_helperSource, nestedLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);

    const quint64 removal = backend->removeBreakpoint(backend->numberOf(1));
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(removal), s_timeoutMs);
    QVERIFY(backend->answeredOk(removal));

    // The loop runs twice more. Had the breakpoint stayed, the next stop would
    // be there rather than in the later function.
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedFile(), m_helperSource);
    QCOMPARE(backend->stoppedLine(), nestedLine);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::doesNotStopAtADisabledBreakpoint()
{
    const int loopLine = lineOf(m_inferiorSource, "sum-loop");
    const int nestedLine = lineOf(m_helperSource, "nested-body");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, loopLine);
    backend->addInitialBreakpoint(m_helperSource, nestedLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), loopLine);

    const quint64 update = backend->setBreakpointEnabled(backend->numberOf(1), m_inferiorSource,
                                                         loopLine, false);
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(update), s_timeoutMs);
    QVERIFY(backend->answeredOk(update));

    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), nestedLine);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
}

void tst_jdwp::runsToALine()
{
    const int startLine = lineOf(m_inferiorSource, "main-start");
    const int targetLine = lineOf(m_inferiorSource, "call-nested");
    const auto backend = launch();
    backend->addInitialBreakpoint(m_inferiorSource, startLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);

    ExecutionRequest request;
    request.command = ExecutionCommand::RunToLine;
    request.context.fileName = m_inferiorSource;
    request.context.textPosition = {targetLine, -1};
    backend->execute(request);
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 2 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), targetLine);
    QVERIFY(backend->contains(InferiorEvent::RunOk));

    // The breakpoint behind it is gone again, and was never the model's.
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stops(), qsizetype(2));
}

void tst_jdwp::interruptsARunningProgram()
{
    const int spinLine = lineOf(m_inferiorSource, "spin-body");
    const auto backend = launch({"spin"});
    backend->addInitialBreakpoint(m_inferiorSource, spinLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), spinLine);

    // Let it spin without the breakpoint, where only an interrupt stops it.
    const quint64 removal = backend->removeBreakpoint(backend->numberOf(1));
    QTRY_VERIFY_WITH_TIMEOUT(backend->answered(removal), s_timeoutMs);
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::RunOk), s_timeoutMs);
    backend->clearStoppedLocation();

    backend->execute(ExecutionCommand::Interrupt);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::StopOk), s_timeoutMs);
    QCOMPARE(backend->stoppedFile(), m_inferiorSource);
    QVERIFY2(backend->stoppedLine() == spinLine || backend->stoppedLine() == spinLine - 1,
             qPrintable(QString::number(backend->stoppedLine())));

    // It goes on from there.
    backend->execute(ExecutionCommand::Continue);
    QTRY_VERIFY_WITH_TIMEOUT(backend->count(InferiorEvent::RunOk) == 2, s_timeoutMs);

    backend->shutdownInferior(ShutdownMode::Kill);
    QTRY_VERIFY_WITH_TIMEOUT(backend->contains(InferiorEvent::ShutdownFinished), s_timeoutMs);
    QVERIFY(backend->results().isEmpty());
}

void tst_jdwp::attachesAndDetaches()
{
    Process vm;
    QString output;
    quint16 port = 0;
    startListeningVm(vm, output, port);
    if (QTest::currentTestFailed())
        return;

    const int nestedLine = lineOf(m_helperSource, "nested-body");
    DebuggerBackend backend(JdwpImplStartData{
        .inferiorStartData = AttachToRemoteServerData{QString("127.0.0.1:%1").arg(port), {}},
        .sourceSearchPaths = {m_sourceRoot}});
    backend.addInitialBreakpoint(m_helperSource, nestedLine);
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.stops() == 1 || !backend.results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend.stoppedFile(), m_helperSource);
    QCOMPARE(backend.stoppedLine(), nestedLine);

    // The program goes on by itself once the debugger has let go of it.
    backend.execute(ExecutionCommand::Detach);
    QTRY_VERIFY_WITH_TIMEOUT(!backend.results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend.results().first().exitStatus, InferiorExitStatus::Detached);
    QTRY_VERIFY_WITH_TIMEOUT(!vm.isRunning(), s_timeoutMs);
    QCOMPARE(vm.exitCode(), 3);
    QVERIFY2(output.contains("result 49 6 12 hello 3"), qPrintable(output));
    QCOMPARE(backend.stops(), qsizetype(1));
}

void tst_jdwp::detachesFromALaunchedProgram()
{
    const int spinLine = lineOf(m_inferiorSource, "spin-body");
    auto backend = launch({"spin"});
    backend->addInitialBreakpoint(m_inferiorSource, spinLine);
    backend->start();
    QTRY_VERIFY_WITH_TIMEOUT(backend->stops() == 1 || !backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->stoppedLine(), spinLine);

    backend->execute(ExecutionCommand::Detach);
    QTRY_VERIFY_WITH_TIMEOUT(!backend->results().isEmpty(), s_timeoutMs);
    QCOMPARE(backend->results().first().exitStatus, InferiorExitStatus::Detached);
    backend->shutdownEngine();
    backend.reset();

    // The program spins on without the debugger.
    const QList<Process *> released = Utils::shutdownGuard()->findChildren<Process *>();
    QCOMPARE(released.size(), 1);
    QPointer<Process> vm = released.first();
    QVERIFY(vm->isRunning());
    vm->kill();
    QTRY_VERIFY_WITH_TIMEOUT(!vm, s_timeoutMs);
}

void tst_jdwp::detachesFromAnAgentThatHangsUp()
{
    FakeAgent agent;
    DebuggerBackend backend(JdwpImplStartData{
        .inferiorStartData = AttachToRemoteServerData{QString("127.0.0.1:%1").arg(agent.port()),
                                                      {}}});
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.contains(InferiorEvent::RunAndInferiorRunOk), s_timeoutMs);

    backend.execute(ExecutionCommand::Detach);
    QTRY_VERIFY_WITH_TIMEOUT(!backend.results().isEmpty(), s_timeoutMs);
    QVERIFY(agent.hungUp());
    QCOMPARE(backend.results().first().exitStatus, InferiorExitStatus::Detached);
}

void tst_jdwp::answersTheCommandsThatWereStillOut()
{
    // The agent hangs up on the question about the threads rather than
    // answering it, so nothing is ever going to come back for it.
    FakeAgent agent(Jdwp::VirtualMachineSet, Jdwp::VmAllThreads);
    DebuggerBackend backend(JdwpImplStartData{
        .inferiorStartData = AttachToRemoteServerData{QString("127.0.0.1:%1").arg(agent.port()),
                                                      {}}});
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.contains(InferiorEvent::RunAndInferiorRunOk), s_timeoutMs);

    // The view still hears back, with what little is known, rather than being
    // left to wait for the rest of the session.
    const quint64 request = backend.refresh(RefreshKind::Threads);
    QTRY_VERIFY_WITH_TIMEOUT(backend.refreshed(request), s_timeoutMs);
    QVERIFY(agent.hungUp());
    QCOMPARE(int(backend.refreshData(request)["threads"].childCount()), 0);
}

// A set of events carries one suspension of the virtual machine, however many
// events it holds, so one resume is what gives it back.
void tst_jdwp::givesBackOneSuspensionPerEventSet()
{
    FakeAgent agent;
    DebuggerBackend backend(JdwpImplStartData{
        .inferiorStartData = AttachToRemoteServerData{QString("127.0.0.1:%1").arg(agent.port()),
                                                      {}}});
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(backend.contains(InferiorEvent::RunAndInferiorRunOk), s_timeoutMs);

    agent.sendStartAndClassPrepare();
    QTRY_VERIFY_WITH_TIMEOUT(agent.resumes() >= 1, s_timeoutMs);
    // Commands are answered in the order they were sent, so another resume
    // would have been here before the answer to this one.
    const quint64 request = backend.refresh(RefreshKind::Threads);
    QTRY_VERIFY_WITH_TIMEOUT(backend.refreshed(request), s_timeoutMs);
    QCOMPARE(agent.resumes(), 1);
}

// A machine speaks of its start before it has said how wide its handles are,
// and they are not the eight bytes a reader would otherwise assume.
void tst_jdwp::readsAnEventThatCameBeforeTheHandleSizes()
{
    FakeAgent agent(Jdwp::VirtualMachineSet, Jdwp::VmDispose, 4);
    agent.sendOnHandshake(agent.startAndClassPrepare());

    JdwpClient client;
    QList<JdwpEventSet> sets;
    bool connected = false;
    QString failure;
    connect(&client, &JdwpClient::eventSetReceived, this,
            [&sets](const JdwpEventSet &set) { sets.append(set); });
    connect(&client, &JdwpClient::connected, this, [&connected] { connected = true; });
    connect(&client, &JdwpClient::disconnected, this, [&failure](const QString &reason) {
        failure = reason;
    });
    client.connectToHost("127.0.0.1", agent.port());
    QTRY_VERIFY_WITH_TIMEOUT(connected || !failure.isEmpty(), s_timeoutMs);
    QVERIFY2(connected, qPrintable(failure));
    QCOMPARE(client.idSizes().objectId, 4);
    // Read with the sizes the machine gave, and there before the session is
    // called connected, whenever they arrived.
    QCOMPARE(sets.size(), 1);
    QCOMPARE(sets.first().events.size(), 2);
    QCOMPARE(sets.first().events.first().kind, quint8(Jdwp::VmStartEvent));
    QCOMPARE(sets.first().events.last().signature,
             QString("L%1;").arg(QString(s_mainClass).replace('.', '/')));
    client.close();
}

// What holds a machine at its start is the suspension its event set came with,
// not what the set says: one that cannot be read must not let it run off.
void tst_jdwp::holdsASuspendedMachineUntilItIsSetUp()
{
    FakeAgent agent;
    agent.sendOnHandshake(agent.unreadableEvent());
    DebuggerBackend backend(JdwpImplStartData{
        .inferiorStartData = AttachToRemoteServerData{QString("127.0.0.1:%1").arg(agent.port()),
                                                      {}}});
    backend.start();
    QTRY_VERIFY_WITH_TIMEOUT(agent.resumes() >= 1, s_timeoutMs);
    QVERIFY(backend.contains(InferiorEvent::EngineSetupOk));
    // The requests of the session go out as it is set up, ahead of the resume
    // that lets the machine go.
    QVERIFY(agent.requestsBeforeTheFirstResume() > 0);
    const quint64 request = backend.refresh(RefreshKind::Threads);
    QTRY_VERIFY_WITH_TIMEOUT(backend.refreshed(request), s_timeoutMs);
    QCOMPARE(agent.resumes(), 1);
}

QTEST_GUILESS_MAIN(tst_jdwp)

#include "tst_jdwp.moc"
