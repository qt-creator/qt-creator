// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "jdwpclient.h"
#include "jdwpexpression.h"

#include "../debuggerengineinterface.h"

#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QElapsedTimer>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QTcpServer>

#include <functional>
#include <memory>
#include <optional>

namespace Debugger::Internal {

// The directory the package statement of a source names, "pkg/sub" for
// "package pkg.sub;", and nothing where the source names no package. Kotlin
// ends the statement with the line rather than with a semicolon.
DEBUGGER_EXPORT QString packageDirectoryOf(const QString &source);

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
        QString genericSignature;
        qint32 modBits = 0;
        bool lineTableKnown = false;
        // Code index and line, in the order of the code.
        QList<QPair<quint64, int>> lines;
        bool variablesKnown = false;
        // Compiled without the table that names them, which is why there are
        // none rather than because the method has none.
        bool variablesAbsent = false;
        class Variable
        {
        public:
            quint64 codeIndex = 0;
            QString name;
            QString signature;
            QString genericSignature;
            qint32 length = 0;
            qint32 slot = 0;
        };
        QList<Variable> variables;
    };

    class FieldInfo
    {
    public:
        quint64 id = 0;
        quint64 declaringClass = 0;
        QString name;
        QString signature;
        QString genericSignature;
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
        // The toString() an object of the class is shown by, and the class
        // that declares it. Both stay zero where nothing but Object declares
        // one, since what Object answers is the identity shown anyway.
        bool toStringKnown = false;
        quint64 toStringClass = 0;
        quint64 toStringMethod = 0;
        // Itself, its superclasses, then its interfaces.
        bool supertypesKnown = false;
        QList<quint64> supertypes;
    };

    // What an expression came out as: a value and the type it is declared with,
    // or why it could not be read.
    class Evaluated
    {
    public:
        JdwpValue value;
        QString type;
        // The type again, as the machine spells it, which is what Java picks
        // an overload by. Empty where it is a type variable or not known, and
        // then the class of the value stands in for it. A null with none is
        // of the null type, which every reference type takes.
        QString signature;
        QString error;
    };
    using Evaluation = std::function<void(const Evaluated &)>;

    // A method a call could mean, and what it takes and answers.
    class Callable
    {
    public:
        quint64 declaringClass = 0;
        quint64 methodId = 0;
        // The signatures of what it takes, and of what it answers.
        QStringList parameters;
        QString returns;
        // What it answers is of a type variable, which tells nothing about
        // the class of what comes back.
        bool returnsTypeVariable = false;
        bool isStatic = false;
        bool isVarArgs = false;
    };

    // What a call is made on: the class its method is looked for in, and the
    // object it goes to, which is none where only a static method will do.
    class Receiver
    {
    public:
        quint64 searchType = 0;
        quint64 object = 0;
        // The object is a null, which only a static method can be called on.
        bool isNull = false;
    };

    // The stop a call is asked for at. A call can only go out while the
    // program is still there and the thread is still the one selected.
    class Stop
    {
    public:
        int runGeneration = 0;
        quint64 thread = 0;
    };

    // What a call into the virtual machine came back with: what the method
    // answered, or what it threw instead, or nothing because it never came
    // back at all.
    class Invoked
    {
    public:
        JdwpValue value;
        JdwpValue thrown;
        bool gaveUp = false;
        QString error;
    };
    using Returned = std::function<void(const Invoked &)>;

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
        class Location
        {
        public:
            // "3.1"
            QString number;
            JdwpLocation where;
            quint64 fieldClass = 0;
            quint64 fieldId = 0;
            QString function;
            Utils::FilePath file;
            int line = 0;
            bool enabled = true;
            qint32 request = 0;
            // Stable across a change of the breakpoint.
            QString key() const;
        };

        QString number;
        // What it stops at: a line of a file, a method of a class, a write to
        // a field, or every exception that is thrown.
        BreakpointType type = BreakpointByFileAndLine;
        Utils::FilePath file;
        QString sourceName;
        // The directory of the package, as in a class signature.
        QString packagePath;
        bool packageKnown = false;
        int line = 0;
        // Where the code nearest to the line is, which is where it stops.
        int actualLine = 0;
        QString className;
        QString memberName;
        bool enabled = true;
        // Worked out here at every hit, since the protocol has no conditions
        // and its count modifier stops the request rather than ignoring it.
        QString condition;
        int ignoreCount = 0;
        int hits = 0;
        int ignoredSince = 0;
        // A thread object id, or -1 for any.
        qint64 threadSpec = -1;
        bool tracepoint = false;
        QString message;
        bool oneShot = false;
        // Behind a run to a line, and invisible to the model.
        bool internal = false;
        // Whether the model has heard about it yet.
        bool answered = false;
        qint32 classPrepareRequest = 0;
        QSet<quint64> classes;
        // Only an exception breakpoint has one.
        QList<qint32> requests;
        QList<Location> locations;
        int nextLocationNumber = 1;
        // By Location::key().
        QSet<QString> disabledLocations;
    };

    // The breakpoint is looked up while the requests are still out, as a
    // one-shot gives its own up when spent. Empty for a stop of no breakpoint.
    class Hit
    {
    public:
        QString number;
        JdwpEvent event;
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
    void pin(const JdwpValue &value, const Stop &stop);
    Stop currentStop() const { return {m_runGeneration, m_currentThread}; }
    // Why a call cannot go out at the stop it was asked for at, or nothing
    // where it can.
    QString whyNoCallAt(const Stop &stop) const;
    void reportRunOk();
    void reportStop(const QList<JdwpEvent> &hits);
    void reportStopAt(const QList<JdwpEvent> &hits);
    void reportStopReached(const QList<JdwpEvent> &hits, const QList<Hit> &taken, bool requested,
                           int generation);
    void reportException(const JdwpEvent &event);
    // Whether a hit is one to stop at: its condition holds, and it is not one
    // of the first that are to be ignored.
    void decideOnHit(const JdwpEvent &event, const std::function<void(bool)> &done);
    void decideOnHits(QList<JdwpEvent> hits, QList<JdwpEvent> taken,
                      const std::function<void(const QList<JdwpEvent> &)> &done);
    // Whether an exception was both thrown and caught inside the runtime, and
    // so is none the program can be stopped at on its account.
    void isRuntimesOwn(const JdwpEvent &event, const std::function<void(bool)> &done);
    Breakpoint *breakpointOfRequest(quint8 eventKind, qint32 requestId);
    Breakpoint *breakpointBeingArmedFor(const JdwpEvent &event);
    Breakpoint *breakpointOfHit(const JdwpEvent &event);
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
    void withToString(quint64 classId, const Done &done);
    const MethodInfo *method(quint64 classId, quint64 methodId) const;
    void resolveLocation(const JdwpLocation &location,
                         const std::function<void(const ResolvedLocation &)> &done);
    Utils::FilePath localFile(const QString &signature, const QString &sourceFile) const;

    class Modifiers
    {
    public:
        void add(const QByteArray &modifier) { ++count; data += modifier; }
        qint32 count = 0;
        QByteArray data;
    };

    // Breakpoints, which cannot be set before their class is loaded and are
    // set in each class the source file or the class name turns out to have.
    Breakpoint *breakpoint(const QString &number);
    Breakpoint::Location *breakpointLocation(const QString &number);
    GdbMi breakpointData(const Breakpoint &bp) const;
    GdbMi locationData(const Breakpoint &bp, const Breakpoint::Location &location) const;
    GdbMi breakpointReport(const Breakpoint &bp) const;
    void armBreakpoint(const QString &number, const Done &done);
    void disarmBreakpoint(const QString &number, const Done &done);
    // Keeps the resolved locations.
    void clearRequestsOf(const QString &number, const Done &done);
    Modifiers commonModifiers(const Breakpoint &bp) const;
    void armLocation(const QString &number, const QString &locationNumber, const Done &done);
    void dropLocation(const QString &locationNumber, const QString &reason);
    void enableLocation(const QString &locationNumber, bool enabled, const Done &done);
    QString addLocation(Breakpoint &bp, const Breakpoint::Location &location);
    void resolveInClass(const QString &number, quint64 classId, const Done &done);
    void resolveLineInClass(const QString &number, quint64 classId, const Done &done);
    void resolveMethodInClass(const QString &number, quint64 classId, const Done &done);
    void resolveFieldInClass(const QString &number, quint64 classId, const Done &done);
    void handleClassPrepares(const QList<JdwpEvent> &events, const Done &done);
    void updateFromBreakpointRequest(Breakpoint &bp, const BreakpointParameters &params);
    void reportTracepoint(const QString &number, const JdwpEvent &event, const Done &done);
    void reportTracepoints(QList<Hit> hits, const Done &done);
    // A capture whose expression calls, which is worked out after the ones
    // read off the frames: the call would make the machine forget them.
    class CallingCapture
    {
    public:
        qsizetype index = 0;
        JdwpExpression parsed;
    };
    void answerTracepointCalls(const std::shared_ptr<QStringList> &filled,
                               const std::shared_ptr<QList<CallingCapture>> &calling,
                               int generation, const Done &done);
    void reportFieldWrite(const Hit &hit);
    void valueText(const JdwpValue &value, const std::function<void(const QString &)> &done);

    // The stack of the current thread while it is stopped.
    void withFrames(int count, const Done &done);
    void reportStack(quint64 requestId, int depthLimit);
    void reportThreads(quint64 requestId);
    // What the machine was started with, which is what it has in place of the
    // libraries a native program is made of.
    void reportModules(quint64 requestId);
    // The sources of the classes that are the program's rather than the
    // runtime's, which are the ones a user has any use for.
    void reportSourceFiles(quint64 requestId);

    // Expressions. Everything but a call is read off the suspended thread and
    // changes nothing about what the program does; a call is the exception,
    // and only runs where the views allow it.
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
    // The class a dotted name stands for, or zero where no loaded class goes
    // by it. A plain name is tried in the package of the frame and in
    // java.lang as well, the way a source that imports neither would read it.
    void classNamed(const QString &name, const std::function<void(quint64)> &done);
    void classNamedAmong(const QStringList &candidates, qsizetype index,
                         const std::function<void(quint64)> &done);
    // The loaded class of that signature, or zero.
    void classOfSignature(const QString &signature, const std::function<void(quint64)> &done);
    void withObjectClass(const std::function<void(quint64)> &done);
    void withAllClasses(const Done &done);
    void classNameOf(const JdwpValue &object, const std::function<void(const QString &)> &done);
    // The class of the object, or 0 where the machine does not say.
    void withClassOf(quint64 object, const std::function<void(quint64)> &done);

    // Calls, which are what makes an expression more than a read. The method
    // is chosen the way JLS 15.12 has it, by the static types of the receiver
    // and of the arguments, and the machine's dispatch picks the override.
    void callOf(const JdwpExpression &expression, const Evaluation &done);
    void callOn(const Stop &stop, const Receiver &receiver, const QString &name,
                const QList<Evaluated> &arguments, const Evaluation &done);
    // The class an unqualified call is one of: the innermost of the frame's
    // class and those around it with a method of the name, and the object of
    // it the frame has, if any.
    void enclosingReceiver(quint64 classId, quint64 object, const QString &name,
                           const std::function<void(const Receiver &)> &done);
    // The arguments of a call, worked out left to right as Java has them, and
    // answered with what went wrong where one of them could not be.
    using Arguments = QList<std::shared_ptr<JdwpExpression>>;
    void evaluateEach(const Arguments &expressions, qsizetype index,
                      const std::shared_ptr<QList<Evaluated>> &values,
                      const std::function<void(const QString &error)> &done);
    // The methods of the class and of everything it inherits from that go by
    // the name, each declaration ahead of those it overrides or hides.
    void withCallables(quint64 classId, const QString &name,
                       const std::function<void(const QList<Callable> &)> &done);
    void gatherCallables(const QList<quint64> &types, qsizetype index, const QString &name,
                         const std::shared_ptr<QList<Callable>> &found,
                         const std::function<void(const QList<Callable> &)> &done);
    // The type, its superclasses, and then the interfaces of all of them, each
    // ahead of those it extends.
    using Types = std::function<void(const QList<quint64> &)>;
    class SupertypeWalk;
    void withSupertypes(quint64 typeId, const Types &done);
    void expandSupertypes(const std::shared_ptr<SupertypeWalk> &walk, const Types &done);
    void withSignatures(const QList<quint64> &typeIds, qsizetype index, const Done &done);
    // What each of the types is a subtype of, by signature, and itself.
    using Subtyping = QHash<QString, QSet<QString>>;
    void withSupertypeSignatures(const QStringList &signatures, qsizetype index,
                                 const std::shared_ptr<Subtyping> &subtyping, const Done &done);
    // The static type of each argument, with the class of the value where
    // the type is not known.
    void withArgumentSignatures(const QList<Evaluated> &arguments, qsizetype index,
                                const std::shared_ptr<QStringList> &signatures,
                                const Done &done);
    // The argument as the parameter takes it: boxed, unboxed or widened.
    void converted(const Stop &stop, const Evaluated &argument, const QString &parameter,
                   const std::function<void(const JdwpValue &, const QString &error)> &done);
    void convertedEach(const Stop &stop, const QList<Evaluated> &arguments,
                       const QStringList &parameters, qsizetype index,
                       const std::shared_ptr<QList<JdwpValue>> &values,
                       const std::function<void(const QString &error)> &done);
    // The array the trailing arguments of a variable arity call go in.
    void newArray(const Stop &stop, const QString &signature, const QList<JdwpValue> &elements,
                  const std::function<void(const JdwpValue &, const QString &error)> &done);
    enum class Dispatch { OnObject, Static };
    void invoke(const Stop &stop, quint64 object, quint64 classId, quint64 methodId,
                Dispatch dispatch, const QList<JdwpValue> &arguments, const Returned &done);

    // Assignment, which is the one thing a debugger does to a value rather
    // than with it.
    void placeOf(const JdwpExpression &expression, const Placement &done);
    void placeOfName(const QString &name, const Placement &done);
    void placeOfStaticField(quint64 classId, const QString &name, const Placement &done);
    void placeOfFieldIn(quint64 object, const QString &name, const Placement &done);
    void writeTo(const Place &place, const JdwpValue &value, const QString &shown);

    // What an object shows itself as, which is the one thing here that makes
    // the program run: toString() is called in the virtual machine, on the
    // thread that is stopped and with the others left suspended.
    class ToStringCall
    {
    public:
        QString iname;
        quint64 object = 0;
        quint64 declaringClass = 0;
        quint64 methodId = 0;
    };
    void showAsToString(const JoinPtr &join, const QString &iname, quint64 object,
                        quint64 classId);
    // Runs what the views put off until everything that reads off a frame was
    // read: the watchers that call, and then the toString() of every object
    // shown. One at a time, since the machine takes no second call on a
    // thread that is off running the first.
    void answerCalls(int generation, const Stop &stop);
    // Hands over what the call returned, and whether it was given up on rather
    // than answered.
    using Called = std::function<void(const std::optional<QString> &text, bool gaveUp)>;
    void invokeToString(const Stop &stop, quint64 object, quint64 declaringClass,
                        quint64 methodId, const Called &done);

    // A watcher whose expression calls, which cannot be worked out while the
    // frames are being read: the call would make the machine forget them.
    class DeferredWatcher
    {
    public:
        QString iname;
        QString expression;
        JdwpExpression parsed;
    };

    void fetchLocals(const RefreshRequest &request);
    void addWatchers(const JoinPtr &join, const QJsonArray &watchers);
    // The statics of the class the frame stands in, under one row of their
    // own, since they belong to no local and to no object.
    void addStaticFields(const JoinPtr &join);
    // Says once why a method shows no locals where the reason is that it was
    // compiled without the table that names them.
    void mentionAbsentVariables(const JdwpLocation &location);
    void addValue(const JoinPtr &join, const QString &iname, const QString &name,
                  const QString &declaredType, const JdwpValue &value);
    void addObjectValue(const JoinPtr &join, const QString &iname, const JdwpValue &value);
    QString expressionOf(const QString &iname) const;
    GdbMi localsItem(const QString &iname) const;
    void reportLocals();
    void runPendingCommands();

    const JdwpImplStartData m_startData;
    JdwpClient m_client;
    QTcpServer m_server;
    std::unique_ptr<Utils::Process> m_process;
    // For $TICK.
    QElapsedTimer m_since;

    bool m_setupReported = false;
    bool m_runReported = false;
    bool m_suspendedForSetup = false;
    bool m_running = false;
    bool m_interruptRequested = false;
    bool m_decidingOnHit = false;
    bool m_runRequestPending = false;
    bool m_shuttingDown = false;
    bool m_shutdownReported = false;
    bool m_detaching = false;
    bool m_inferiorDoneReported = false;
    // The threads a call the views made is out on, by how many are out on
    // each. A call is out until the machine answers it and not until it is
    // waited for: one given up on still has the thread it runs on, and one
    // made on that thread afterwards is refused rather than answered. A stop
    // of such a thread while a call is out is none of the user's, and is run
    // past rather than reported, once with a word about it.
    QHash<quint64, int> m_callsOut;
    bool m_callStopMentioned = false;
    // The objects an expression made or got back from a call, which nothing in
    // the program need refer to while the next call runs the thread. They are
    // kept from being collected until the program is let go.
    QList<quint64> m_pinned;
    int m_extraSuspends = 0;
    // Stops that other threads ran into while one was being reported. Each
    // holds a suspension of its own, and comes next.
    QList<QList<JdwpEvent>> m_pendingStops;

    QHash<quint64, ClassInfo> m_classes;
    // The stop every class the machine had loaded was read at, which is what
    // the Source Files view lists. Classes load while the program runs, so it
    // is read again at the next stop, and every time while it runs.
    int m_allClassesFrom = -1;
    quint64 m_objectClass = 0;
    // The files the breakpoints were set in, by their path below the package root.
    QHash<QString, Utils::FilePath> m_knownSources;
    QHash<QString, QList<Done>> m_waiting;
    // The methods whose missing variable table has been mentioned, so that it
    // is said once rather than at every stop in them.
    QSet<QString> m_absentMentioned;

    QMap<QString, Breakpoint> m_breakpoints;
    int m_nextBreakpointNumber = 1;
    QList<qint32> m_stepRequests;
    // By event kind. A hit of one may still be on its way. Kept until the
    // machine hands the number out again.
    QSet<QPair<quint8, qint32>> m_givenUpRequests;

    quint64 m_currentThread = 0;
    int m_currentFrame = 0;
    QList<Frame> m_frames;
    bool m_framesComplete = false;
    // A call into the virtual machine lets the thread run, and every frame
    // handle read before it stops being valid. This counts the calls, and the
    // frames remember the count they were read at.
    int m_callGeneration = 0;
    int m_framesFrom = 0;
    // Changes with every resume and with every thread selected, which makes
    // what was fetched before it stale.
    int m_stopGeneration = 0;
    // Changes with every resume alone, which is what tells a stop that is
    // still being decided on that the machine was let go without it.
    int m_runGeneration = 0;

    quint64 m_localsRequestId = 0;
    // What the views last asked for, which a write makes stale. Whether they
    // allow the program to be made to run is part of it, and holds for the
    // views alone: everything else that calls was asked for by the user.
    RefreshRequest m_lastLocalsRequest;
    int m_localsGeneration = 0;
    int m_reportedLocalsGeneration = 0;
    // Typed into the console while the views' calls were still being made.
    QStringList m_pendingCommands;
    // The run generation the console's expression is being worked out at, if
    // any. Selecting another thread does not end it, only going on does.
    int m_commandRunGeneration = -1;
    // Per stop and per class of the frame, misses included, with the call
    // generation they were looked up at.
    QHash<QString, std::pair<quint64, int>> m_classesNamed;
    int m_classesNamedGeneration = -1;
    QSet<QString> m_expandedINames;
    QJsonObject m_expandedItems;
    QMap<QString, Local> m_locals;
    QStringList m_localRoots;
    QList<ToStringCall> m_toStringCalls;
    QList<DeferredWatcher> m_deferredWatchers;
};

} // namespace Debugger::Internal
