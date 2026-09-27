// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../debugger_global.h"

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

#include <functional>

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace Debugger::Internal {

// The Java Debug Wire Protocol, as far as a debugger front end needs it:
// https://docs.oracle.com/en/java/javase/21/docs/specs/jdwp/jdwp-protocol.html
namespace Jdwp {

enum CommandSet : quint8 {
    VirtualMachineSet = 1,
    ReferenceTypeSet = 2,
    ClassTypeSet = 3,
    MethodSet = 6,
    ObjectReferenceSet = 9,
    StringReferenceSet = 10,
    ThreadReferenceSet = 11,
    ArrayReferenceSet = 13,
    EventRequestSet = 15,
    StackFrameSet = 16,
    EventSet = 64,
};

enum VirtualMachineCommand : quint8 {
    VmVersion = 1,
    VmAllClasses = 3,
    VmAllThreads = 4,
    VmDispose = 6,
    VmIdSizes = 7,
    VmSuspend = 8,
    VmResume = 9,
    VmExit = 10,
    VmCreateString = 11,
    VmCapabilitiesNew = 17,
};

enum ReferenceTypeCommand : quint8 {
    RefTypeSignature = 1,
    RefTypeGetValues = 6,
    RefTypeSourceFile = 7,
    RefTypeFieldsWithGeneric = 14,
    RefTypeMethodsWithGeneric = 15,
};

enum ClassTypeCommand : quint8 {
    ClassTypeSuperclass = 1,
    ClassTypeSetValues = 2,
};

enum MethodCommand : quint8 {
    MethodLineTable = 1,
    MethodVariableTableWithGeneric = 5,
};

enum ObjectReferenceCommand : quint8 {
    ObjectReferenceType = 1,
    ObjectGetValues = 2,
    ObjectSetValues = 3,
    ObjectInvokeMethod = 6,
};

// How a method the debugger calls is run. Single-threaded leaves the rest of
// the virtual machine suspended, so that a call cannot let the program get
// anywhere the user did not ask it to go.
enum InvokeOption : qint32 { InvokeSingleThreaded = 0x01, InvokeNonVirtual = 0x02 };

enum StringReferenceCommand : quint8 {
    StringValue = 1,
};

enum ThreadReferenceCommand : quint8 {
    ThreadName = 1,
    ThreadStatus = 4,
    ThreadFrames = 6,
    ThreadFrameCount = 7,
};

enum ArrayReferenceCommand : quint8 {
    ArrayLength = 1,
    ArrayGetValues = 2,
    ArraySetValues = 3,
};

enum EventRequestCommand : quint8 {
    EventRequestSetCommand = 1,
    EventRequestClear = 2,
};

enum StackFrameCommand : quint8 {
    StackFrameGetValues = 1,
    StackFrameSetValues = 2,
    StackFrameThisObject = 3,
};

enum EventCommand : quint8 {
    EventComposite = 100,
};

enum EventKind : quint8 {
    SingleStepEvent = 1,
    BreakpointEvent = 2,
    FramePopEvent = 3,
    ExceptionEvent = 4,
    UserDefinedEvent = 5,
    ThreadStartEvent = 6,
    ThreadDeathEvent = 7,
    ClassPrepareEvent = 8,
    ClassUnloadEvent = 9,
    ClassLoadEvent = 10,
    FieldAccessEvent = 20,
    FieldModificationEvent = 21,
    ExceptionCatchEvent = 30,
    MethodEntryEvent = 40,
    MethodExitEvent = 41,
    MethodExitWithReturnValueEvent = 42,
    MonitorContendedEnterEvent = 43,
    MonitorContendedEnteredEvent = 44,
    MonitorWaitEvent = 45,
    MonitorWaitedEvent = 46,
    VmStartEvent = 90,
    VmDeathEvent = 99,
};

enum SuspendPolicy : quint8 {
    SuspendNone = 0,
    SuspendEventThread = 1,
    SuspendAll = 2,
};

enum ModifierKind : quint8 {
    CountModifier = 1,
    ThreadOnlyModifier = 3,
    ClassOnlyModifier = 4,
    ClassMatchModifier = 5,
    ClassExcludeModifier = 6,
    LocationOnlyModifier = 7,
    ExceptionOnlyModifier = 8,
    StepModifier = 10,
    SourceNameMatchModifier = 12,
};

enum StepSize { StepMin = 0, StepLine = 1 };
enum StepDepth { StepInto = 0, StepOver = 1, StepOut = 2 };

enum TypeTag : quint8 { ClassTag = 1, InterfaceTag = 2, ArrayTag = 3 };

enum Tag : quint8 {
    ArrayValueTag = '[',
    ByteValueTag = 'B',
    CharValueTag = 'C',
    ObjectValueTag = 'L',
    FloatValueTag = 'F',
    DoubleValueTag = 'D',
    IntValueTag = 'I',
    LongValueTag = 'J',
    ShortValueTag = 'S',
    VoidValueTag = 'V',
    BooleanValueTag = 'Z',
    StringValueTag = 's',
    ThreadValueTag = 't',
    ThreadGroupValueTag = 'g',
    ClassLoaderValueTag = 'l',
    ClassObjectValueTag = 'c',
};

enum Error : quint16 {
    NoError = 0,
    InvalidThreadError = 10,
    ThreadNotSuspendedError = 13,
    InvalidObjectError = 20,
    InvalidFrameIdError = 30,
    NotImplementedError = 99,
    AbsentInformationError = 101,
    VmDeadError = 112,
};

enum ThreadStatusValue {
    ThreadZombie = 0,
    ThreadRunning = 1,
    ThreadSleeping = 2,
    ThreadMonitor = 3,
    ThreadWait = 4,
};

enum ModifierBits { StaticModifierBit = 0x0008, AbstractModifierBit = 0x0400 };

// The indices into the answer to CapabilitiesNew.
enum Capability { CanUseSourceNameFilters = 18 };

bool isObjectTag(quint8 tag);
bool isPrimitiveTag(quint8 tag);

} // namespace Jdwp

// How wide the handles of this virtual machine are. The protocol leaves that to
// the implementation, which says so in its answer to IDSizes.
class DEBUGGER_EXPORT JdwpIdSizes
{
public:
    int fieldId = 8;
    int methodId = 8;
    int objectId = 8;
    int referenceTypeId = 8;
    int frameId = 8;
};

class DEBUGGER_EXPORT JdwpLocation
{
public:
    bool isValid() const { return classId != 0; }
    bool operator==(const JdwpLocation &other) const = default;

    quint8 typeTag = 0;
    quint64 classId = 0;
    quint64 methodId = 0;
    quint64 index = 0;
};

// A value as the wire carries it: the tag says what the bits are, which is an
// object id for everything that is not a primitive. A signed number is held
// sign-extended, so that reading the bits as one needs no width.
class DEBUGGER_EXPORT JdwpValue
{
public:
    bool isObject() const { return Jdwp::isObjectTag(tag); }
    bool isNull() const { return isObject() && bits == 0; }

    quint8 tag = Jdwp::VoidValueTag;
    quint64 bits = 0;
};

class DEBUGGER_EXPORT JdwpWriter
{
public:
    explicit JdwpWriter(const JdwpIdSizes &sizes) : m_sizes(sizes) {}

    JdwpWriter &writeByte(quint8 value);
    JdwpWriter &writeBool(bool value);
    JdwpWriter &writeInt(qint32 value);
    JdwpWriter &writeLong(qint64 value);
    JdwpWriter &writeString(const QString &value);
    JdwpWriter &writeObjectId(quint64 value);
    JdwpWriter &writeReferenceTypeId(quint64 value);
    JdwpWriter &writeMethodId(quint64 value);
    JdwpWriter &writeFieldId(quint64 value);
    JdwpWriter &writeFrameId(quint64 value);
    JdwpWriter &writeLocation(const JdwpLocation &location);
    // A value where its type is known from elsewhere, as a field or an array
    // element holds it, and one that carries its tag along.
    JdwpWriter &writeUntaggedValue(const JdwpValue &value);
    JdwpWriter &writeTaggedValue(const JdwpValue &value);

    const QByteArray &data() const { return m_data; }

private:
    JdwpWriter &writeSized(quint64 value, int size);

    JdwpIdSizes m_sizes;
    QByteArray m_data;
};

// Reads what a reply or an event carries. A read past the end yields zero and
// marks the reader as failed, so a caller checks once, after the last read.
class DEBUGGER_EXPORT JdwpReader
{
public:
    JdwpReader(const QByteArray &data, const JdwpIdSizes &sizes)
        : m_data(data), m_sizes(sizes) {}

    quint8 readByte();
    bool readBool();
    qint32 readInt();
    qint64 readLong();
    QString readString();
    quint64 readObjectId();
    quint64 readReferenceTypeId();
    quint64 readMethodId();
    quint64 readFieldId();
    quint64 readFrameId();
    JdwpLocation readLocation();
    JdwpValue readTaggedValue();
    JdwpValue readUntaggedValue(quint8 tag);

    bool ok() const { return m_ok; }
    bool atEnd() const { return m_pos >= m_data.size(); }

private:
    quint64 readSized(int size);

    QByteArray m_data;
    JdwpIdSizes m_sizes;
    qsizetype m_pos = 0;
    bool m_ok = true;
};

class DEBUGGER_EXPORT JdwpReply
{
public:
    bool ok() const { return errorCode == Jdwp::NoError; }

    quint16 errorCode = Jdwp::NoError;
    QByteArray data;
};

class DEBUGGER_EXPORT JdwpEvent
{
public:
    quint8 kind = 0;
    qint32 requestId = 0;
    quint64 thread = 0;
    JdwpLocation location;
    // What a class prepare event names.
    quint8 refTypeTag = 0;
    quint64 typeId = 0;
    QString signature;
    // What an exception event carries in addition to the location.
    JdwpValue exception;
    JdwpLocation catchLocation;
};

class DEBUGGER_EXPORT JdwpEventSet
{
public:
    quint8 suspendPolicy = Jdwp::SuspendNone;
    QList<JdwpEvent> events;
};

class DEBUGGER_EXPORT JdwpClient final : public QObject
{
    Q_OBJECT

public:
    using ReplyHandler = std::function<void(const JdwpReply &reply)>;

    explicit JdwpClient(QObject *parent = nullptr);
    ~JdwpClient() override;

    // Takes the socket over and shakes hands on it. connected() follows once
    // the virtual machine has said how wide its handles are.
    void setSocket(QTcpSocket *socket);
    void connectToHost(const QString &host, quint16 port);
    void close();
    bool isConnected() const { return m_connected; }

    const JdwpIdSizes &idSizes() const { return m_idSizes; }
    bool hasCapability(Jdwp::Capability capability) const;

    JdwpWriter writer() const { return JdwpWriter(m_idSizes); }
    JdwpReader reader(const QByteArray &data) const { return JdwpReader(data, m_idSizes); }

    // Sends a command. The handler runs with the reply; commands are answered
    // in the order they were sent.
    quint32 send(quint8 commandSet, quint8 command, const QByteArray &data = {},
                 const ReplyHandler &handler = {});

    static QString errorString(quint16 errorCode);
    static QString commandName(quint8 commandSet, quint8 command);

signals:
    void connected();
    void eventSetReceived(const JdwpEventSet &events);
    void disconnected(const QString &reason);
    void logMessage(const QString &text, bool outgoing);

private:
    void handleReadyRead();
    void handlePacket(const QByteArray &packet);
    void handleEventPacket(const QByteArray &data);
    void startSession();
    void fail(const QString &reason);

    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    bool m_handshakeDone = false;
    bool m_connected = false;
    bool m_closed = false;
    quint32 m_nextId = 1;
    JdwpIdSizes m_idSizes;
    bool m_idSizesKnown = false;
    // The events that came before the handle sizes were known.
    QList<QByteArray> m_earlyEvents;
    QList<bool> m_capabilities;

    class PendingCommand
    {
    public:
        quint8 commandSet = 0;
        quint8 command = 0;
        ReplyHandler handler;
    };
    QHash<quint32, PendingCommand> m_pending;
};

} // namespace Debugger::Internal
