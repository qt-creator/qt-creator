// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "processenums.h"

#include <QDataStream>
#include <QProcess>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {

enum class LauncherPacketType {
    // client -> launcher packets:
    Shutdown,
    StartProcess,
    WriteIntoProcess,
    ControlProcess,
    // launcher -> client packets:
    ProcessStarted,
    ReadyReadStandardOutput,
    ReadyReadStandardError,
    ProcessDone
};

class PacketParser
{
public:
    class InvalidPacketSizeException
    {
    public:
        InvalidPacketSizeException(int size) : size(size) { }
        const int size;
    };

    void setDevice(QIODevice *device);
    bool parse();
    LauncherPacketType type() const { return m_type; }
    quintptr token() const { return m_token; }
    const QByteArray &packetData() const { return m_packetData; }

private:
    QDataStream m_stream;
    LauncherPacketType m_type = LauncherPacketType::Shutdown;
    quintptr m_token = 0;
    QByteArray m_packetData;
    int m_sizeOfNextPacket = -1;
};

class LauncherPacket
{
public:
    virtual ~LauncherPacket();

    template<class Packet> static Packet extractPacket(quintptr token, const QByteArray &data)
    {
        Packet p(token);
        p.deserialize(data);
        return p;
    }

    QByteArray serialize() const;
    void deserialize(const QByteArray &data);

    const LauncherPacketType type;
    const quintptr token = 0;

protected:
    LauncherPacket(LauncherPacketType type, quintptr token) : type(type), token(token) { }

private:
    virtual void doSerialize(QDataStream &stream) const = 0;
    virtual void doDeserialize(QDataStream &stream) = 0;
};

class StartProcessPacket : public LauncherPacket
{
public:
    StartProcessPacket(quintptr token);

    QString command;
    QStringList arguments;
    QString workingDir;
    QStringList env;
    ProcessMode processMode = ProcessMode::Reader;
    QByteArray writeData;
    QProcess::ProcessChannelMode processChannelMode = QProcess::SeparateChannels;
    QString standardInputFile;
    bool belowNormalPriority = false;
    QString nativeArguments;
    bool lowPriority = false;
    bool unixTerminalDisabled = false;
    bool useCtrlCStub = false;
    int reaperTimeout = 500;
    bool createConsoleOnWindows = false;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ProcessStartedPacket : public LauncherPacket
{
public:
    ProcessStartedPacket(quintptr token);

    int processId = 0;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ControlProcessPacket : public LauncherPacket
{
public:
    ControlProcessPacket(quintptr token);

    enum class SignalType {
        Kill, // Calls QProcess::kill
        Terminate, // Calls QProcess::terminate
        Close, // Puts the process into the reaper, no confirmation signal is being sent.
        CloseWriteChannel
    };

    SignalType signalType = SignalType::Kill;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class WritePacket : public LauncherPacket
{
public:
    WritePacket(quintptr token) : LauncherPacket(LauncherPacketType::WriteIntoProcess, token) { }

    QByteArray inputData;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ShutdownPacket : public LauncherPacket
{
public:
    ShutdownPacket();

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ReadyReadPacket : public LauncherPacket
{
public:
    QByteArray standardChannel;

protected:
    ReadyReadPacket(LauncherPacketType type, quintptr token) : LauncherPacket(type, token) { }

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ReadyReadStandardOutputPacket : public ReadyReadPacket
{
public:
    ReadyReadStandardOutputPacket(quintptr token)
        : ReadyReadPacket(LauncherPacketType::ReadyReadStandardOutput, token) { }
};

class ReadyReadStandardErrorPacket : public ReadyReadPacket
{
public:
    ReadyReadStandardErrorPacket(quintptr token)
        : ReadyReadPacket(LauncherPacketType::ReadyReadStandardError, token) { }
};

class ProcessDonePacket : public LauncherPacket
{
public:
    ProcessDonePacket(quintptr token);

    QByteArray stdOut;
    QByteArray stdErr;

    int exitCode = 0;
    QProcess::ExitStatus exitStatus = QProcess::NormalExit;
    QProcess::ProcessError error = QProcess::UnknownError;
    QString errorString;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

} // namespace Internal
} // namespace Utils

Q_DECLARE_METATYPE(Utils::Internal::LauncherPacketType);
