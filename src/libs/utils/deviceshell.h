/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "utils_global.h"

#include <QMap>
#include <QMutex>
#include <QProcess>
#include <QThread>
#include <QWaitCondition>

#include <memory>

namespace Utils {

class CommandLine;
class ProcessResultData;
class QtcProcess;

class DeviceShellImpl;

class QTCREATOR_UTILS_EXPORT DeviceShell : public QObject
{
    Q_OBJECT

public:
    enum class State { FailedToStart = -1, Unknown = 0, Succeeded = 1 };

    struct RunResult
    {
        int exitCode = 0;
        QByteArray stdOut;
        QByteArray stdErr;
    };

    enum class ParseType {
        StdOut,
        StdErr,
        ExitCode,
    };

    DeviceShell();
    virtual ~DeviceShell();

    bool start();

    bool runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});
    RunResult outputForRunInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    State state() const;

signals:
    void done(const ProcessResultData &resultData);

protected:
    virtual void startupFailed(const CommandLine &cmdLine);
    RunResult run(const CommandLine &cmd, const QByteArray &stdInData = {});

    void close();

private:
    virtual void setupShellProcess(QtcProcess *shellProcess);

    bool installShellScript();
    void closeShellProcess();

    void onReadyRead();

private:
    struct CommandRun : public RunResult
    {
        QWaitCondition *waiter;
    };

    std::unique_ptr<QtcProcess> m_shellProcess;
    QThread m_thread;
    int m_currentId{0};

    QMutex m_commandMutex;
    // QMap is used here to preserve iterators
    QMap<quint64, CommandRun> m_commandOutput;
    QByteArray m_commandBuffer;

    State m_shellScriptState = State::Unknown;
};

} // namespace Utils
