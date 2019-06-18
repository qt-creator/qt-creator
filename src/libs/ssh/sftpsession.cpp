/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sftpsession.h"

#include "sshlogging_p.h"
#include "sshprocess.h"
#include "sshsettings.h"

#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QByteArrayList>
#include <QFileInfo>
#include <QQueue>
#include <QTimer>

using namespace Utils;

namespace QSsh {
using namespace Internal;

enum class CommandType { Ls, Mkdir, Rmdir, Rm, Rename, Ln, Put, Get, None };

struct Command
{
    Command() = default;
    Command(CommandType t, const QStringList &p, SftpJobId id) : type(t), paths(p), jobId(id) {}
    bool isValid() const { return type != CommandType::None; }

    CommandType type = CommandType::None;
    QStringList paths;
    SftpJobId jobId = SftpInvalidJob;
};

struct SftpSession::SftpSessionPrivate
{
    SshProcess sftpProc;
    QStringList connectionArgs;
    QByteArray output;
    QQueue<Command> pendingCommands;
    Command activeCommand;
    SftpJobId nextJobId = 1;
    SftpSession::State state = SftpSession::State::Inactive;

    QByteArray commandString(CommandType command) const
    {
        switch (command) {
        case CommandType::Ls: return "ls -n";
        case CommandType::Mkdir: return "mkdir";
        case CommandType::Rmdir: return "rmdir";
        case CommandType::Rm: return "rm";
        case CommandType::Rename: return "rename";
        case CommandType::Ln: return "ln -s";
        case CommandType::Put: return "put";
        case CommandType::Get: return "get";
        default: QTC_ASSERT(false, return QByteArray());
        }
    }

    SftpJobId queueCommand(CommandType command, const QStringList &paths)
    {
        qCDebug(sshLog) << "queueing command" << int(command) << paths;

        const SftpJobId jobId = nextJobId++;
        pendingCommands.enqueue(Command(command, paths, jobId));
        runNextCommand();
        return jobId;
    }

    void runNextCommand()
    {
        if (activeCommand.isValid())
            return;
        if (pendingCommands.empty())
            return;
        QTC_ASSERT(sftpProc.state() == QProcess::Running, return);
        activeCommand = pendingCommands.dequeue();

        // The second newline forces the prompt to appear after the command has finished.
        sftpProc.write(commandString(activeCommand.type) + ' '
                       + QtcProcess::Arguments::createUnixArgs(activeCommand.paths)
                       .toString().toLocal8Bit() + "\n\n");
    }
};

static QByteArray prompt() { return "sftp> "; }

SftpSession::SftpSession(const QStringList &connectionArgs) : d(new SftpSessionPrivate)
{
    d->connectionArgs = connectionArgs;
    connect(&d->sftpProc, &QProcess::started, [this] {
        qCDebug(sshLog) << "sftp process started";
        d->sftpProc.write("\n"); // Force initial prompt.
    });
    connect(&d->sftpProc, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            d->state = State::Inactive;
            emit done(tr("sftp failed to start: %1").arg(d->sftpProc.errorString()));
        }
    });
    connect(&d->sftpProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this] {
        qCDebug(sshLog) << "sftp process finished";

        d->state = State::Inactive;
        if (d->sftpProc.exitStatus() != QProcess::NormalExit) {
            emit done(tr("sftp crashed."));
            return;
        }
        if (d->sftpProc.exitCode() != 0) {
            emit done(QString::fromLocal8Bit(d->sftpProc.readAllStandardError()));
            return;
        }
        emit done(QString());
    });
    connect(&d->sftpProc, &QProcess::readyReadStandardOutput, this, &SftpSession::handleStdout);
}

void SftpSession::doStart()
{
    if (d->state != State::Starting)
        return;
    const FilePath sftpBinary = SshSettings::sftpFilePath();
    if (!sftpBinary.exists()) {
        d->state = State::Inactive;
        emit done(tr("Cannot establish SFTP session: sftp binary \"%1\" does not exist.")
                  .arg(sftpBinary.toUserOutput()));
        return;
    }
    d->activeCommand = Command();
    const QStringList args = QStringList{"-q"} << d->connectionArgs;
    qCDebug(sshLog) << "starting sftp session:" << sftpBinary.toUserOutput() << args;
    d->sftpProc.start(sftpBinary.toString(), args);
}

void SftpSession::handleStdout()
{
    if (state() == State::Running && !d->activeCommand.isValid()) {
        qCWarning(sshLog) << "ignoring unexpected sftp output:"
                        << d->sftpProc.readAllStandardOutput();
        return;
    }

    d->output += d->sftpProc.readAllStandardOutput();
    qCDebug(sshLog) << "accumulated sftp output:" << d->output;
    const int firstPromptOffset = d->output.indexOf(prompt());
    if (firstPromptOffset == -1)
        return;
    if (state() == State::Starting) {
        d->state = State::Running;
        d->output.clear();
        d->sftpProc.readAllStandardError(); // The "connected" message goes to stderr.
        emit started();
        return;
    }
    const int secondPromptOffset = d->output.indexOf(prompt(), firstPromptOffset + prompt().size());
    if (secondPromptOffset == -1)
        return;
    const Command command = d->activeCommand;
    d->activeCommand = Command();
    const QByteArray commandOutput = d->output.mid(
                firstPromptOffset + prompt().size(),
                secondPromptOffset - firstPromptOffset - prompt().size());
    d->output = d->output.mid(secondPromptOffset + prompt().size());
    if (command.type == CommandType::Ls)
        handleLsOutput(command.jobId, commandOutput);
    const QByteArray stdErr = d->sftpProc.readAllStandardError();
    emit commandFinished(command.jobId, QString::fromLocal8Bit(stdErr));
    d->runNextCommand();
}

static SftpFileType typeFromLsOutput(char c)
{
    if (c == '-')
        return FileTypeRegular;
    if (c == 'd')
        return FileTypeDirectory;
    return FileTypeOther;
}

static QFile::Permissions permissionsFromLsOutput(const QByteArray &output)
{
    QFile::Permissions perms;
    if (output.at(0) == 'r')
        perms |= QFile::ReadOwner;
    if (output.at(1) == 'w')
        perms |= QFile::WriteOwner;
    if (output.at(2) == 'x')
        perms |= QFile::ExeOwner;
    if (output.at(3) == 'r')
        perms |= QFile::ReadGroup;
    if (output.at(4) == 'w')
        perms |= QFile::WriteGroup;
    if (output.at(5) == 'x')
        perms |= QFile::ExeGroup;
    if (output.at(6) == 'r')
        perms |= QFile::ReadOther;
    if (output.at(7) == 'w')
        perms |= QFile::WriteOther;
    if (output.at(8) == 'x')
        perms |= QFile::ExeOther;
    return perms;
}

void SftpSession::handleLsOutput(SftpJobId jobId, const QByteArray &output)
{
    QList<SftpFileInfo> allFileInfo;
    for (const QByteArray &line : output.split('\n')) {
        if (line.startsWith("ls") || line.isEmpty())
            continue;
        const QByteArrayList components = line.simplified().split(' ');
        if (components.size() < 9) {
            qCWarning(sshLog) << "Don't know how to parse sftp ls output:" << line;
            continue;
        }
        const QByteArray typeAndPermissions = components.first();
        if (typeAndPermissions.size() != 10) {
            qCWarning(sshLog) << "Don't know how to parse sftp ls output:" << line;
            continue;
        }
        SftpFileInfo fileInfo;
        fileInfo.type = typeFromLsOutput(typeAndPermissions.at(0));
        fileInfo.permissions = permissionsFromLsOutput(QByteArray::fromRawData(
                                                           typeAndPermissions.constData() + 1,
                                                           typeAndPermissions.size() - 1));
        bool isNumber;
        fileInfo.size = components.at(4).toULongLong(&isNumber);
        if (!isNumber) {
            qCWarning(sshLog) << "Don't know how to parse sftp ls output:" << line;
            continue;
        }
        // TODO: This will not work for file names with weird whitespace combinations
        fileInfo.name = QFileInfo(QString::fromUtf8(components.mid(8).join(' '))).fileName();
        allFileInfo << fileInfo;
    }
    emit fileInfoAvailable(jobId, allFileInfo);
}

SftpSession::~SftpSession()
{
    quit();
    delete d;
}

void SftpSession::start()
{
    QTC_ASSERT(d->state == State::Inactive, return);
    d->state = State::Starting;
    QTimer::singleShot(0, this, &SftpSession::doStart);
}

void SftpSession::quit()
{
    qCDebug(sshLog) << "quitting sftp session, current state is" << int(state());

    switch (state()) {
    case State::Starting:
    case State::Closing:
        d->state = State::Closing;
        d->sftpProc.kill();
        break;
    case State::Running:
        d->state = State::Closing;
        d->sftpProc.write("bye\n");
        break;
    case State::Inactive:
        break;
    }
}

SftpJobId SftpSession::ls(const QString &path)
{
    return d->queueCommand(CommandType::Ls, QStringList(path));
}

SftpJobId SftpSession::createDirectory(const QString &path)
{
    return d->queueCommand(CommandType::Mkdir, QStringList(path));
}

SftpJobId SftpSession::removeDirectory(const QString &path)
{
    return d->queueCommand(CommandType::Rmdir, QStringList(path));
}

SftpJobId SftpSession::removeFile(const QString &path)
{
    return d->queueCommand(CommandType::Rm, QStringList(path));
}

SftpJobId SftpSession::rename(const QString &oldPath, const QString &newPath)
{
    return d->queueCommand(CommandType::Rename, QStringList{oldPath, newPath});
}

SftpJobId SftpSession::createSoftLink(const QString &filePath, const QString &target)
{
    return d->queueCommand(CommandType::Ln, QStringList{filePath, target});
}

SftpJobId SftpSession::uploadFile(const QString &localFilePath, const QString &remoteFilePath)
{
    return d->queueCommand(CommandType::Put, QStringList{localFilePath, remoteFilePath});
}

SftpJobId SftpSession::downloadFile(const QString &remoteFilePath, const QString &localFilePath)
{
    return d->queueCommand(CommandType::Get, QStringList{remoteFilePath, localFilePath});
}

SftpSession::State SftpSession::state() const
{
    return d->state;
}

} // namespace QSsh
