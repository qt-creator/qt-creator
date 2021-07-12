/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QtCore/qdatastream.h>
#include <QtCore/qprocess.h>
#include <QtCore/qstringlist.h>

QT_BEGIN_NAMESPACE
class QByteArray;
QT_END_NAMESPACE

namespace Utils {
namespace Internal {

enum class LauncherPacketType {
    Shutdown, StartProcess, StopProcess, ProcessError, ProcessFinished
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
    QProcess::ProcessChannelMode mode = QProcess::SeparateChannels;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class StopProcessPacket : public LauncherPacket
{
public:
    StopProcessPacket(quintptr token);

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

class ProcessErrorPacket : public LauncherPacket
{
public:
    ProcessErrorPacket(quintptr token);

    QProcess::ProcessError error = QProcess::UnknownError;
    QString errorString;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

class ProcessFinishedPacket : public LauncherPacket
{
public:
    ProcessFinishedPacket(quintptr token);

    QString errorString;
    QByteArray stdOut;
    QByteArray stdErr;
    QProcess::ExitStatus exitStatus = QProcess::ExitStatus::NormalExit;
    QProcess::ProcessError error = QProcess::ProcessError::UnknownError;
    int exitCode = 0;

private:
    void doSerialize(QDataStream &stream) const override;
    void doDeserialize(QDataStream &stream) override;
};

} // namespace Internal
} // namespace Utils

Q_DECLARE_METATYPE(Utils::Internal::LauncherPacketType);
