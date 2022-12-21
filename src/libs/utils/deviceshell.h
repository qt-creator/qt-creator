// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "fileutils.h"

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
    enum class State { FailedToStart = -1, Unknown = 0, Succeeded = 1, NoScript = 2 };

    enum class ParseType {
        StdOut,
        StdErr,
        ExitCode,
    };

    DeviceShell(bool forceFailScriptInstallation = false);
    virtual ~DeviceShell();

    bool start();

    RunResult runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    State state() const;

    QStringList missingFeatures() const;

signals:
    void done(const ProcessResultData &resultData);

protected:
    virtual void startupFailed(const CommandLine &cmdLine);
    RunResult run(const CommandLine &cmd, const QByteArray &stdInData = {});

    void close();

private:
    virtual void setupShellProcess(QtcProcess *shellProcess);
    virtual CommandLine createFallbackCommand(const CommandLine &cmdLine);

    bool installShellScript();
    void closeShellProcess();

    void onReadyRead();

    bool checkCommand(const QByteArray &command);

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
    QStringList m_missingFeatures;

    // Only used for tests
    bool m_forceFailScriptInstallation = false;
};

} // namespace Utils
