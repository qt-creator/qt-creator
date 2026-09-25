// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jdwpclient.h"
#include "jdwpexpression.h"

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QTcpServer>

#include <functional>
#include <memory>

namespace Debugger::Internal {

class DEBUGGER_EXPORT JdwpImplStartData
{
public:
    // A launch runs the virtual machine with its debug agent pointed at a port
    // opened here. An attach reaches an agent that listens already, at the
    // "host:port" of the channel.
    InferiorStartData inferiorStartData;
    // Where the sources are, as the roots of their package directories. The
    // working directory of a launch is looked in last.
    QList<Utils::FilePath> sourceSearchPaths;
    // The sources of the project, which is where a class is looked for first.
    QList<Utils::FilePath> sourceFiles;
};

// A backend that is no debugger process but the virtual machine itself,
// reached over the Java Debug Wire Protocol.
class DEBUGGER_EXPORT JdwpImpl final : public DebuggerEngineInterface
{
    Q_OBJECT

public:
    explicit JdwpImpl(const JdwpImplStartData &startData);
    ~JdwpImpl() override;

private:
    using Done = std::function<void()>;

    // Calls its callback once everything that was added has been released,
    // and never if an answer it waits for does not come.
    class Join
    {
    public:
        explicit Join(Done done) : m_done(std::move(done)) {}
        void add() { ++m_pending; }
        void release();

    private:
        int m_pending = 0;
        Done m_done;
    };
    using JoinPtr = std::shared_ptr<Join>;

    class MethodInfo
    {
    public:
        quint64 id = 0;
        QString name;
        QString signature;
        qint32 modBits = 0;
        bool lineTableKnown = false;
        // Code index and line, in the order of the code.
        QList<QPair<quint64, int>> lines;
        bool variablesKnown = false;
        class Variable
        {
        public:
            quint64 codeIndex = 0;
            QString name;
            QString signature;
            qint32 length = 0;
            qint32 slot = 0;
        };
        QList<Variable> variables;
    };

    class FieldInfo
    {
    public:
        quint64 id = 0;
        QString name;
        QString signature;
        qint32 modBits = 0;
    };

    class ClassInfo
    {
    public:
        quint8 tag = Jdwp::ClassTag;
        bool signatureKnown = false;
        QString signature;
        bool sourceFileKnown = false;
        QString sourceFile;
        bool methodsKnown = false;
        QList<MethodInfo> methods;
        // The instance fields of the class and of all its superclasses.
        bool fieldsKnown = false;
        QList<FieldInfo> fields;
        // Its static ones and those of its superclasses, closest first.
        QList<FieldInfo> staticFields;
    };

    // What an expression came out as: a value and the type it is declared with,
    // or why it could not be read.
    class Evaluated
    {
    public:
        JdwpValue value;
        QString type;
        QString error;
    };
    using Evaluation = std::function<void(const Evaluated &)>;

    // Where a value lives, which is what an assignment needs and reading one
    // does not. The signature is the type the place is declared with, and what
    // the value assigned to it is made to fit.
    class Place
    {
    public:
        enum class Kind { Local, Field, StaticField, Element };
        Kind kind = Kind::Local;
        // The frame for a local, the object for a field, the class for a static
        // one, the array for an element.
        quint64 owner = 0;
        quint64 fieldId = 0;
        // The slot of a local, or the index of an element.
        qint32 position = 0;
        QString signature;
        QString error;
    };
    using Placement = std::function<void(const Place &)>;

    class ResolvedLocation
    {
    public:
        QString function;
        Utils::FilePath file;
        int line = 0;
    };

    class Frame
    {
    public:
        quint64 id = 0;
        JdwpLocation location;
    };

    class Breakpoint
    {
    public:
        QString number;
        Utils::FilePath file;
        QString sourceName;
        // The directory of the package, as in a class signature.
        QString packagePath;
        bool packageKnown = false;
        int line = 0;
        // Where the code nearest to the line is, which is where it stops.
        int actualLine = 0;
        bool enabled = true;
        // Behind a run to a line, and invisible to the model.
        bool internal = false;
        // Whether the model has heard about it yet.
        bool answered = false;
        qint32 classPrepareRequest = 0;
        QSet<quint64> classes;
        QList<qint32> requests;
        QList<JdwpLocation> locations;
    };

    class Local
    {
    public:
        QString name;
        QString type;
        QString value;
        bool hasChildren = false;
        QStringList children;
        // The expression a watcher stands for, hex-encoded, which is how the
        // model finds its way back to the row the user typed.
        QString watcherName;
        QString exp;
    };

    void start() final;
    void shutdownInferior(ShutdownMode mode) final;
    void shutdownEngine() final;

    void execute(const ExecutionRequest &request) final;
    void changeBreakpoint(const BreakpointChangeRequest &request) final;
    void refresh(const RefreshRequest &request) final;

    void selectThread(const QString &threadId) final;
    void activateFrame(int index) final;
    void setRegisterValue(const QString &name, const QString &value) final;
    void accessMemory(MemoryOp op, quint64 requestId, quint64 addr, quint64 lengthOrSize,
                      const QByteArray &data) final;
    void fetchDisassembly(quint64 requestId, quint64 address, const QString &functionName) final;
    void setPeripheralRegisterValue(quint64 address, quint64 value) final;
    void watchPoint(quint64 requestId, const QPoint &pnt) final;
    void createSnapshot(quint64 requestId) final;
    void assignValueInDebugger(const WatchItemData &item, const QString &expr,
                               const QString &value) final;
    void executeDebuggerCommand(const QString &command,
                                const WatchItemData &inspectorItem) final;

    bool isLaunch() const;
    void launch();
    void attach();
    void handleConnected();
    void handleDisconnected(const QString &reason);
    void handleProcessDone();
    void releaseProcess();
    void handleEventSet(const JdwpEventSet &set);
    void reportUnsupported(const QString &what);
    void reportInferiorDone(const InferiorResultData &result);
    void reportShutdownFinished();

    void send(quint8 commandSet, quint8 command, const QByteArray &data = {},
              const JdwpClient::ReplyHandler &handler = {});
    JdwpWriter writer() const { return m_client.writer(); }
    JdwpReader reader(const QByteArray &data) const { return m_client.reader(data); }

    // Running and stopping. Every stop holds one suspension of the virtual
    // machine, and a resume gives back exactly those it holds.
    void resumeFromStop();
    void reportRunOk();
    void reportStop(quint64 thread, const JdwpLocation &location);
    void reportStopAt(quint64 thread, const JdwpLocation &location);
    void stopAfterInterrupt();
    void step(Jdwp::StepDepth depth, bool byInstruction);
    void runToLine(const ContextData &context);
    void clearTransientRequests();
    void clearRequest(quint8 eventKind, qint32 requestId, const Done &done = {});

    // What is known about the classes, fetched once each.
    void fetchOnce(const QString &key, const std::function<bool()> &known,
                   const std::function<void(const Done &finish)> &fetch, const Done &done);
    void withSignature(quint64 classId, const Done &done);
    void withSourceFile(quint64 classId, const Done &done);
    void withMethods(quint64 classId, const Done &done);
    void withLineTable(quint64 classId, quint64 methodId, const Done &done);
    void withVariables(quint64 classId, quint64 methodId, const Done &done);
    void withFields(quint64 classId, const Done &done);
    const MethodInfo *method(quint64 classId, quint64 methodId) const;
    void resolveLocation(const JdwpLocation &location,
                         const std::function<void(const ResolvedLocation &)> &done);
    Utils::FilePath localFile(const QString &signature, const QString &sourceFile) const;

    // Breakpoints, which cannot be set before their class is loaded and are
    // set in each class the source file turns out to have.
    Breakpoint *breakpoint(const QString &number);
    GdbMi breakpointData(const Breakpoint &bp) const;
    void armBreakpoint(const QString &number, const Done &done);
    void disarmBreakpoint(const QString &number, const Done &done);
    void resolveInClass(const QString &number, quint64 classId, const Done &done);
    void handleClassPrepares(const QList<JdwpEvent> &events, const Done &done);
    void updateFromBreakpointRequest(Breakpoint &bp, const BreakpointParameters &params);

    // The stack of the current thread while it is stopped.
    void withFrames(int count, const Done &done);
    void reportStack(quint64 requestId, int depthLimit);
    void reportThreads(quint64 requestId);

    // Expressions, as far as reading the virtual machine answers them: no call
    // runs in it, so nothing a watcher shows can change what the program does.
    void evaluate(const JdwpExpression &expression, const Evaluation &done);
    void evaluateName(const QString &name, const Evaluation &done);
    void evaluateThis(const Evaluation &done);
    void compareWithLiteral(const QString &literal, const JdwpExpression &other, bool equal,
                            const Evaluation &done);
    void staticFieldOf(quint64 classId, const QString &className, const QString &name,
                       const Evaluation &done);
    void fieldOf(const Evaluated &base, const QString &name, const Evaluation &done);
    void elementOf(const Evaluated &base, const Evaluated &index, const Evaluation &done);
    void withCurrentFrame(const std::function<void(const Frame *)> &done);
    QList<MethodInfo::Variable> visibleVariables(const JdwpLocation &location) const;

    // Assignment, which is the one thing a debugger does to a value rather
    // than with it.
    void placeOf(const JdwpExpression &expression, const Placement &done);
    void placeOfName(const QString &name, const Placement &done);
    void placeOfStaticField(quint64 classId, const QString &name, const Placement &done);
    void placeOfFieldIn(quint64 object, const QString &name, const Placement &done);
    void writeTo(const Place &place, const JdwpValue &value, const QString &shown);

    void fetchLocals(const RefreshRequest &request);
    void addWatchers(const JoinPtr &join, const QJsonArray &watchers);
    void addValue(const JoinPtr &join, const QString &iname, const QString &name,
                  const QString &declaredType, const JdwpValue &value);
    void addObjectValue(const JoinPtr &join, const QString &iname, const JdwpValue &value);
    QString expressionOf(const QString &iname) const;
    GdbMi localsItem(const QString &iname) const;
    void reportLocals();

    const JdwpImplStartData m_startData;
    JdwpClient m_client;
    QTcpServer m_server;
    std::unique_ptr<Utils::Process> m_process;

    bool m_setupReported = false;
    bool m_runReported = false;
    bool m_suspendedForSetup = false;
    bool m_running = false;
    bool m_interruptRequested = false;
    bool m_runRequestPending = false;
    bool m_shuttingDown = false;
    bool m_shutdownReported = false;
    bool m_detaching = false;
    bool m_inferiorDoneReported = false;
    int m_extraSuspends = 0;
    // Stops that other threads ran into while one was being reported. Each
    // holds a suspension of its own, and comes next.
    QList<JdwpEvent> m_pendingStops;

    QHash<quint64, ClassInfo> m_classes;
    // The files the breakpoints were set in, by their path below the package root.
    QHash<QString, Utils::FilePath> m_knownSources;
    QHash<QString, QList<Done>> m_waiting;

    QMap<QString, Breakpoint> m_breakpoints;
    int m_nextBreakpointNumber = 1;
    QList<qint32> m_stepRequests;

    quint64 m_currentThread = 0;
    int m_currentFrame = 0;
    QList<Frame> m_frames;
    bool m_framesComplete = false;
    // Changes with every resume, which makes what the stop before it had
    // fetched stale.
    int m_stopGeneration = 0;

    quint64 m_localsRequestId = 0;
    // What the views last asked for, which a write makes stale.
    RefreshRequest m_lastLocalsRequest;
    int m_localsGeneration = 0;
    QSet<QString> m_expandedINames;
    QJsonObject m_expandedItems;
    QMap<QString, Local> m_locals;
    QStringList m_localRoots;
};

} // namespace Debugger::Internal
