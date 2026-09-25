// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jdwpclient.h"

#include "../debuggertr.h"

#include <utils/qtcassert.h>

#include <QTcpSocket>
#include <QtEndian>

#include <algorithm>
#include <utility>

namespace Debugger::Internal {

static const QByteArray s_handshake = "JDWP-Handshake";
static constexpr int s_headerSize = 11;
static constexpr quint8 s_replyFlag = 0x80;
// A class list runs to hundreds of kilobytes, which helps nobody in the log.
static constexpr qsizetype s_loggedBytes = 64;

static QString loggedData(const QByteArray &data)
{
    const QString hex = QString::fromLatin1(data.left(s_loggedBytes).toHex());
    return data.size() > s_loggedBytes
               ? QString("%1... (%2 bytes)").arg(hex).arg(data.size()) : hex;
}

// A command that cannot go out, or one that was still out when the connection
// went, is answered as if the virtual machine were gone. A handler that never
// runs would leave everything waiting behind it waiting forever.
static void reportGone(const JdwpClient::ReplyHandler &handler)
{
    if (handler)
        handler({Jdwp::VmDeadError, {}});
}

namespace Jdwp {

bool isObjectTag(quint8 tag)
{
    switch (tag) {
    case ArrayValueTag:
    case ObjectValueTag:
    case StringValueTag:
    case ThreadValueTag:
    case ThreadGroupValueTag:
    case ClassLoaderValueTag:
    case ClassObjectValueTag:
        return true;
    default:
        return false;
    }
}

bool isPrimitiveTag(quint8 tag)
{
    switch (tag) {
    case ByteValueTag:
    case CharValueTag:
    case FloatValueTag:
    case DoubleValueTag:
    case IntValueTag:
    case LongValueTag:
    case ShortValueTag:
    case BooleanValueTag:
        return true;
    default:
        return false;
    }
}

} // namespace Jdwp

JdwpWriter &JdwpWriter::writeByte(quint8 value)
{
    m_data.append(char(value));
    return *this;
}

JdwpWriter &JdwpWriter::writeBool(bool value)
{
    return writeByte(value ? 1 : 0);
}

JdwpWriter &JdwpWriter::writeInt(qint32 value)
{
    return writeSized(quint32(value), 4);
}

JdwpWriter &JdwpWriter::writeLong(qint64 value)
{
    return writeSized(quint64(value), 8);
}

JdwpWriter &JdwpWriter::writeString(const QString &value)
{
    const QByteArray utf8 = value.toUtf8();
    writeInt(qint32(utf8.size()));
    m_data.append(utf8);
    return *this;
}

JdwpWriter &JdwpWriter::writeObjectId(quint64 value)
{
    return writeSized(value, m_sizes.objectId);
}

JdwpWriter &JdwpWriter::writeReferenceTypeId(quint64 value)
{
    return writeSized(value, m_sizes.referenceTypeId);
}

JdwpWriter &JdwpWriter::writeMethodId(quint64 value)
{
    return writeSized(value, m_sizes.methodId);
}

JdwpWriter &JdwpWriter::writeFieldId(quint64 value)
{
    return writeSized(value, m_sizes.fieldId);
}

JdwpWriter &JdwpWriter::writeFrameId(quint64 value)
{
    return writeSized(value, m_sizes.frameId);
}

JdwpWriter &JdwpWriter::writeLocation(const JdwpLocation &location)
{
    writeByte(location.typeTag);
    writeReferenceTypeId(location.classId);
    writeMethodId(location.methodId);
    return writeLong(qint64(location.index));
}

JdwpWriter &JdwpWriter::writeUntaggedValue(const JdwpValue &value)
{
    switch (value.tag) {
    case Jdwp::ByteValueTag:
    case Jdwp::BooleanValueTag:
        return writeSized(value.bits, 1);
    case Jdwp::CharValueTag:
    case Jdwp::ShortValueTag:
        return writeSized(value.bits, 2);
    case Jdwp::FloatValueTag:
    case Jdwp::IntValueTag:
        return writeSized(value.bits, 4);
    case Jdwp::DoubleValueTag:
    case Jdwp::LongValueTag:
        return writeSized(value.bits, 8);
    case Jdwp::VoidValueTag:
        return *this;
    default:
        break;
    }
    if (Jdwp::isObjectTag(value.tag))
        return writeObjectId(value.bits);
    return *this;
}

JdwpWriter &JdwpWriter::writeTaggedValue(const JdwpValue &value)
{
    writeByte(value.tag);
    return writeUntaggedValue(value);
}

JdwpWriter &JdwpWriter::writeSized(quint64 value, int size)
{
    for (int shift = (size - 1) * 8; shift >= 0; shift -= 8)
        m_data.append(char((value >> shift) & 0xff));
    return *this;
}

quint64 JdwpReader::readSized(int size)
{
    if (m_pos + size > m_data.size()) {
        m_ok = false;
        m_pos = m_data.size();
        return 0;
    }
    quint64 value = 0;
    for (int i = 0; i < size; ++i)
        value = (value << 8) | quint8(m_data.at(m_pos++));
    return value;
}

quint8 JdwpReader::readByte()
{
    return quint8(readSized(1));
}

bool JdwpReader::readBool()
{
    return readByte() != 0;
}

qint32 JdwpReader::readInt()
{
    return qint32(quint32(readSized(4)));
}

qint64 JdwpReader::readLong()
{
    return qint64(readSized(8));
}

QString JdwpReader::readString()
{
    const qint32 size = readInt();
    if (size < 0 || m_pos + size > m_data.size()) {
        m_ok = false;
        m_pos = m_data.size();
        return {};
    }
    const QString result = QString::fromUtf8(m_data.mid(m_pos, size));
    m_pos += size;
    return result;
}

quint64 JdwpReader::readObjectId()
{
    return readSized(m_sizes.objectId);
}

quint64 JdwpReader::readReferenceTypeId()
{
    return readSized(m_sizes.referenceTypeId);
}

quint64 JdwpReader::readMethodId()
{
    return readSized(m_sizes.methodId);
}

quint64 JdwpReader::readFieldId()
{
    return readSized(m_sizes.fieldId);
}

quint64 JdwpReader::readFrameId()
{
    return readSized(m_sizes.frameId);
}

JdwpLocation JdwpReader::readLocation()
{
    JdwpLocation location;
    location.typeTag = readByte();
    location.classId = readReferenceTypeId();
    location.methodId = readMethodId();
    location.index = quint64(readLong());
    return location;
}

JdwpValue JdwpReader::readTaggedValue()
{
    return readUntaggedValue(readByte());
}

JdwpValue JdwpReader::readUntaggedValue(quint8 tag)
{
    JdwpValue value;
    value.tag = tag;
    switch (tag) {
    case Jdwp::BooleanValueTag:
        value.bits = readSized(1);
        break;
    case Jdwp::ByteValueTag:
        value.bits = quint64(qint8(quint8(readSized(1))));
        break;
    case Jdwp::CharValueTag:
        value.bits = readSized(2);
        break;
    case Jdwp::ShortValueTag:
        value.bits = quint64(qint16(quint16(readSized(2))));
        break;
    case Jdwp::FloatValueTag:
        value.bits = readSized(4);
        break;
    case Jdwp::IntValueTag:
        value.bits = quint64(qint32(quint32(readSized(4))));
        break;
    case Jdwp::DoubleValueTag:
    case Jdwp::LongValueTag:
        value.bits = readSized(8);
        break;
    case Jdwp::VoidValueTag:
        break;
    default:
        if (Jdwp::isObjectTag(tag))
            value.bits = readObjectId();
        else
            m_ok = false;
        break;
    }
    return value;
}

JdwpClient::JdwpClient(QObject *parent)
    : QObject(parent)
{}

JdwpClient::~JdwpClient()
{
    if (m_socket)
        m_socket->disconnect(this);
}

void JdwpClient::setSocket(QTcpSocket *socket)
{
    QTC_ASSERT(socket && !m_socket, return);
    m_socket = socket;
    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &JdwpClient::handleReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, [this] {
        fail(Tr::tr("The virtual machine closed the connection."));
    });
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this] {
        fail(m_socket->errorString());
    });
    const auto shakeHands = [this] { m_socket->write(s_handshake); };
    if (m_socket->state() == QAbstractSocket::ConnectedState)
        shakeHands();
    else
        connect(m_socket, &QTcpSocket::connected, this, shakeHands);
}

void JdwpClient::connectToHost(const QString &host, quint16 port)
{
    auto socket = new QTcpSocket;
    setSocket(socket);
    socket->connectToHost(host, port);
}

void JdwpClient::close()
{
    if (m_closed)
        return;
    m_closed = true;
    m_connected = false;
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->abort();
    }
    // In the order the commands went out, which is the order they would have
    // been answered in.
    const QHash<quint32, PendingCommand> pending = std::exchange(m_pending, {});
    QList<quint32> ids = pending.keys();
    std::sort(ids.begin(), ids.end());
    for (const quint32 id : std::as_const(ids))
        reportGone(pending.value(id).handler);
}

bool JdwpClient::hasCapability(Jdwp::Capability capability) const
{
    return m_capabilities.value(capability, false);
}

quint32 JdwpClient::send(quint8 commandSet, quint8 command, const QByteArray &data,
                         const ReplyHandler &handler)
{
    QTC_ASSERT(m_socket && !m_closed, reportGone(handler); return 0);
    const quint32 id = m_nextId++;
    QByteArray packet(s_headerSize, Qt::Uninitialized);
    qToBigEndian<quint32>(quint32(s_headerSize + data.size()), packet.data());
    qToBigEndian<quint32>(id, packet.data() + 4);
    packet[8] = 0;
    packet[9] = char(commandSet);
    packet[10] = char(command);
    packet.append(data);
    m_pending.insert(id, {commandSet, command, handler});
    emit logMessage(QString("%1 %2 %3").arg(id).arg(commandName(commandSet, command))
                        .arg(loggedData(data)), true);
    m_socket->write(packet);
    return id;
}

void JdwpClient::handleReadyRead()
{
    m_buffer.append(m_socket->readAll());

    if (!m_handshakeDone) {
        if (m_buffer.size() < s_handshake.size())
            return;
        if (!m_buffer.startsWith(s_handshake)) {
            fail(Tr::tr("The other side does not speak the Java Debug Wire Protocol."));
            return;
        }
        m_buffer.remove(0, s_handshake.size());
        m_handshakeDone = true;
        startSession();
    }

    while (!m_closed && m_buffer.size() >= s_headerSize) {
        const quint32 length = qFromBigEndian<quint32>(m_buffer.constData());
        if (length < quint32(s_headerSize)) {
            fail(Tr::tr("Received a malformed packet."));
            return;
        }
        if (quint32(m_buffer.size()) < length)
            return;
        const QByteArray packet = m_buffer.left(length);
        m_buffer.remove(0, length);
        handlePacket(packet);
    }
}

void JdwpClient::handlePacket(const QByteArray &packet)
{
    const quint32 id = qFromBigEndian<quint32>(packet.constData() + 4);
    const quint8 flags = quint8(packet.at(8));
    const QByteArray data = packet.mid(s_headerSize);

    if (flags & s_replyFlag) {
        JdwpReply reply;
        reply.errorCode = qFromBigEndian<quint16>(packet.constData() + 9);
        reply.data = data;
        const PendingCommand command = m_pending.take(id);
        emit logMessage(QString("%1 %2 %3").arg(id)
                            .arg(reply.ok() ? QString("ok") : errorString(reply.errorCode))
                            .arg(loggedData(data)), false);
        if (command.handler)
            command.handler(reply);
        return;
    }

    const quint8 commandSet = quint8(packet.at(9));
    const quint8 command = quint8(packet.at(10));
    if (commandSet == Jdwp::EventSet && command == Jdwp::EventComposite) {
        // The virtual machine speaks of its start before it has answered how
        // wide its handles are, and nothing in an event can be read until it
        // has, so such an event waits here for the answer.
        if (m_idSizesKnown)
            handleEventPacket(data);
        else
            m_earlyEvents.append(data);
        return;
    }
    emit logMessage(QString("Ignoring command %1 from the virtual machine")
                        .arg(commandName(commandSet, command)), false);
}

void JdwpClient::handleEventPacket(const QByteArray &data)
{
    JdwpReader reader = this->reader(data);
    JdwpEventSet set;
    set.suspendPolicy = reader.readByte();
    const qint32 count = reader.readInt();
    QStringList kinds;
    for (qint32 i = 0; i < count && reader.ok(); ++i) {
        JdwpEvent event;
        event.kind = reader.readByte();
        event.requestId = reader.readInt();
        kinds.append(QString::number(event.kind));
        switch (event.kind) {
        case Jdwp::VmStartEvent:
        case Jdwp::ThreadStartEvent:
        case Jdwp::ThreadDeathEvent:
            event.thread = reader.readObjectId();
            break;
        case Jdwp::SingleStepEvent:
        case Jdwp::BreakpointEvent:
        case Jdwp::MethodEntryEvent:
        case Jdwp::MethodExitEvent:
            event.thread = reader.readObjectId();
            event.location = reader.readLocation();
            break;
        case Jdwp::MethodExitWithReturnValueEvent:
            event.thread = reader.readObjectId();
            event.location = reader.readLocation();
            reader.readTaggedValue();
            break;
        case Jdwp::ExceptionEvent:
            event.thread = reader.readObjectId();
            event.location = reader.readLocation();
            event.exception = reader.readTaggedValue();
            event.catchLocation = reader.readLocation();
            break;
        case Jdwp::ClassPrepareEvent:
            event.thread = reader.readObjectId();
            event.refTypeTag = reader.readByte();
            event.typeId = reader.readReferenceTypeId();
            event.signature = reader.readString();
            reader.readInt(); // status
            break;
        case Jdwp::ClassUnloadEvent:
            event.signature = reader.readString();
            break;
        case Jdwp::FieldAccessEvent:
        case Jdwp::FieldModificationEvent:
            event.thread = reader.readObjectId();
            event.location = reader.readLocation();
            reader.readByte(); // refTypeTag
            reader.readReferenceTypeId();
            reader.readFieldId();
            reader.readTaggedValue(); // object
            if (event.kind == Jdwp::FieldModificationEvent)
                reader.readTaggedValue();
            break;
        case Jdwp::MonitorContendedEnterEvent:
        case Jdwp::MonitorContendedEnteredEvent:
        case Jdwp::MonitorWaitEvent:
        case Jdwp::MonitorWaitedEvent:
            event.thread = reader.readObjectId();
            reader.readTaggedValue(); // object
            event.location = reader.readLocation();
            if (event.kind == Jdwp::MonitorWaitEvent)
                reader.readLong();
            else if (event.kind == Jdwp::MonitorWaitedEvent)
                reader.readBool();
            break;
        case Jdwp::VmDeathEvent:
            break;
        default:
            // Nothing tells how long an event of an unknown kind is, so the
            // ones behind it cannot be read either.
            emit logMessage(QString("Unknown event kind %1").arg(event.kind), false);
            reader = this->reader({});
            reader.readByte();
            break;
        }
        if (reader.ok())
            set.events.append(event);
    }
    emit logMessage(QString("Event set, suspend policy %1, kinds %2")
                        .arg(set.suspendPolicy).arg(kinds.join(',')), false);
    emit eventSetReceived(set);
}

void JdwpClient::startSession()
{
    send(Jdwp::VirtualMachineSet, Jdwp::VmIdSizes, {}, [this](const JdwpReply &reply) {
        if (!reply.ok()) {
            fail(Tr::tr("The virtual machine did not say how wide its handles are: %1")
                     .arg(errorString(reply.errorCode)));
            return;
        }
        JdwpReader reader = this->reader(reply.data);
        JdwpIdSizes sizes;
        sizes.fieldId = reader.readInt();
        sizes.methodId = reader.readInt();
        sizes.objectId = reader.readInt();
        sizes.referenceTypeId = reader.readInt();
        sizes.frameId = reader.readInt();
        const auto valid = [](int size) { return size > 0 && size <= 8; };
        if (!reader.ok() || !valid(sizes.fieldId) || !valid(sizes.methodId)
            || !valid(sizes.objectId) || !valid(sizes.referenceTypeId)
            || !valid(sizes.frameId)) {
            fail(Tr::tr("The virtual machine reported unusable handle sizes."));
            return;
        }
        m_idSizes = sizes;
        m_idSizesKnown = true;
        const QList<QByteArray> early = std::exchange(m_earlyEvents, {});
        for (const QByteArray &event : early) {
            if (m_closed)
                return;
            handleEventPacket(event);
        }
        send(Jdwp::VirtualMachineSet, Jdwp::VmCapabilitiesNew, {},
             [this](const JdwpReply &reply) {
            if (reply.ok()) {
                JdwpReader reader = this->reader(reply.data);
                while (!reader.atEnd())
                    m_capabilities.append(reader.readBool());
            }
            m_connected = true;
            emit connected();
        });
    });
}

void JdwpClient::fail(const QString &reason)
{
    if (m_closed)
        return;
    close();
    emit disconnected(reason);
}

QString JdwpClient::errorString(quint16 errorCode)
{
    switch (errorCode) {
    case Jdwp::NoError: return "NONE";
    case Jdwp::InvalidThreadError: return "INVALID_THREAD";
    case Jdwp::ThreadNotSuspendedError: return "THREAD_NOT_SUSPENDED";
    case Jdwp::InvalidObjectError: return "INVALID_OBJECT";
    case Jdwp::InvalidFrameIdError: return "INVALID_FRAMEID";
    case Jdwp::NotImplementedError: return "NOT_IMPLEMENTED";
    case Jdwp::AbsentInformationError: return "ABSENT_INFORMATION";
    case Jdwp::VmDeadError: return "VM_DEAD";
    }
    return QString("ERROR %1").arg(errorCode);
}

QString JdwpClient::commandName(quint8 commandSet, quint8 command)
{
    static const QHash<quint16, QString> names = {
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmVersion, "VirtualMachine.Version"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmAllClasses, "VirtualMachine.AllClasses"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmAllThreads, "VirtualMachine.AllThreads"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmDispose, "VirtualMachine.Dispose"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmIdSizes, "VirtualMachine.IDSizes"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmSuspend, "VirtualMachine.Suspend"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmResume, "VirtualMachine.Resume"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmExit, "VirtualMachine.Exit"},
        {Jdwp::VirtualMachineSet << 8 | Jdwp::VmCapabilitiesNew, "VirtualMachine.CapabilitiesNew"},
        {Jdwp::ReferenceTypeSet << 8 | Jdwp::RefTypeSignature, "ReferenceType.Signature"},
        {Jdwp::ReferenceTypeSet << 8 | Jdwp::RefTypeSourceFile, "ReferenceType.SourceFile"},
        {Jdwp::ReferenceTypeSet << 8 | Jdwp::RefTypeFieldsWithGeneric,
         "ReferenceType.FieldsWithGeneric"},
        {Jdwp::ReferenceTypeSet << 8 | Jdwp::RefTypeMethodsWithGeneric,
         "ReferenceType.MethodsWithGeneric"},
        {Jdwp::ClassTypeSet << 8 | Jdwp::ClassTypeSuperclass, "ClassType.Superclass"},
        {Jdwp::MethodSet << 8 | Jdwp::MethodLineTable, "Method.LineTable"},
        {Jdwp::MethodSet << 8 | Jdwp::MethodVariableTableWithGeneric,
         "Method.VariableTableWithGeneric"},
        {Jdwp::ObjectReferenceSet << 8 | Jdwp::ObjectReferenceType,
         "ObjectReference.ReferenceType"},
        {Jdwp::ObjectReferenceSet << 8 | Jdwp::ObjectGetValues, "ObjectReference.GetValues"},
        {Jdwp::StringReferenceSet << 8 | Jdwp::StringValue, "StringReference.Value"},
        {Jdwp::ThreadReferenceSet << 8 | Jdwp::ThreadName, "ThreadReference.Name"},
        {Jdwp::ThreadReferenceSet << 8 | Jdwp::ThreadStatus, "ThreadReference.Status"},
        {Jdwp::ThreadReferenceSet << 8 | Jdwp::ThreadFrames, "ThreadReference.Frames"},
        {Jdwp::ThreadReferenceSet << 8 | Jdwp::ThreadFrameCount, "ThreadReference.FrameCount"},
        {Jdwp::ArrayReferenceSet << 8 | Jdwp::ArrayLength, "ArrayReference.Length"},
        {Jdwp::ArrayReferenceSet << 8 | Jdwp::ArrayGetValues, "ArrayReference.GetValues"},
        {Jdwp::EventRequestSet << 8 | Jdwp::EventRequestSetCommand, "EventRequest.Set"},
        {Jdwp::EventRequestSet << 8 | Jdwp::EventRequestClear, "EventRequest.Clear"},
        {Jdwp::StackFrameSet << 8 | Jdwp::StackFrameGetValues, "StackFrame.GetValues"},
        {Jdwp::StackFrameSet << 8 | Jdwp::StackFrameThisObject, "StackFrame.ThisObject"},
    };
    return names.value(quint16(commandSet << 8 | command),
                       QString("%1.%2").arg(commandSet).arg(command));
}

} // namespace Debugger::Internal
