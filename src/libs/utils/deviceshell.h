// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "expected.h"
#include "fileutils.h"
#include "utils_global.h"

#include <QHash>
#include <QMutex>
#include <QProcess>
#include <QThread>
#include <QWaitCondition>

#include <memory>

namespace Utils {

class CommandLine;
class ProcessResultData;
class Process;

class DeviceShellImpl;

class QTCREATOR_UTILS_EXPORT DeviceShell : public QObject
{
    Q_OBJECT

public:
    enum class State { Failed = -1, Unknown = 0, Succeeded = 1 };

    enum class ParseType {
        StdOut,
        StdErr,
        ExitCode,
    };

    DeviceShell(bool forceFailScriptInstallation = false);
    virtual ~DeviceShell();

    expected_str<void> start();

    RunResult runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    State state() const;

    QStringList missingFeatures() const;

signals:
    void done(const ProcessResultData &resultData);

protected:
    RunResult run(const CommandLine &cmd, const QByteArray &stdInData = {});

    void close();

private:
    virtual void setupShellProcess(Process *shellProcess);
    virtual CommandLine createFallbackCommand(const CommandLine &cmdLine);

    expected_str<void> installShellScript();
    void closeShellProcess();

    void onReadyRead();

    expected_str<QByteArray> checkCommand(const QByteArray &command);

private:
    struct CommandRun : public RunResult
    {
        QWaitCondition *waiter;
    };

    std::unique_ptr<Process> m_shellProcess;
    QThread m_thread;
    int m_currentId{0};

    QMutex m_commandMutex;
    QHash<quint64, CommandRun> m_commandOutput;
    QByteArray m_commandBuffer;

    State m_shellScriptState = State::Unknown;
    QStringList m_missingFeatures;

    // Only used for tests
    bool m_forceFailScriptInstallation = false;
};

} // namespace Utils
