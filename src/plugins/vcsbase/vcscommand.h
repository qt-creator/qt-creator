// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"
#include "vcsenums.h"

#include <coreplugin/progressmanager/processprogress.h>

#include <solutions/tasking/tasktree.h>

#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/textcodec.h>

#include <QObject>

namespace VcsBase {

namespace Internal { class VcsCommandPrivate; }

class VcsCommand;

using ExitCodeInterpreter = std::function<Utils::ProcessResult(int /*exitCode*/)>;

class VCSBASE_EXPORT CommandResult
{
public:
    CommandResult() = default;
    CommandResult(const Utils::Process &process);
    CommandResult(const Utils::Process &process, Utils::ProcessResult result);
    CommandResult(const VcsCommand &command);
    CommandResult(Utils::ProcessResult result, const QString &exitMessage)
        : m_result(result), m_exitMessage(exitMessage) {}

    Utils::ProcessResult result() const { return m_result; }
    int exitCode() const { return m_exitCode; }
    QString exitMessage() const { return m_exitMessage; }

    QString cleanedStdOut() const { return m_cleanedStdOut; }
    QString cleanedStdErr() const { return m_cleanedStdErr; }

    QByteArray rawStdOut() const { return m_rawStdOut; }

private:
    Utils::ProcessResult m_result = Utils::ProcessResult::StartFailed;
    int m_exitCode = 0;
    QString m_exitMessage;

    QString m_cleanedStdOut;
    QString m_cleanedStdErr;

    QByteArray m_rawStdOut;
};

class VCSBASE_EXPORT VcsProcessData
{
public:
    Utils::ProcessRunData runData;
    RunFlags flags = RunFlags::None;
    ExitCodeInterpreter interpreter = {};
    Core::ProgressParser progressParser = {};
    Utils::TextEncoding encoding = {};
    Utils::TextChannelCallback stdOutHandler = {};
    Utils::TextChannelCallback stdErrHandler = {};
};

VCSBASE_EXPORT Tasking::ExecutableItem errorTask(const Utils::FilePath &workingDir,
                                                 const QString &errorMessage);

VCSBASE_EXPORT Utils::ProcessTask vcsProcessTask(const VcsProcessData &data,
    const std::optional<Tasking::Storage<CommandResult>> &resultStorage = {});

class VCSBASE_EXPORT VcsCommand final : public QObject
{
    Q_OBJECT

public:
    // TODO: For master, make c'tor private and make it a friend to VcsBaseClientImpl.
    VcsCommand(const Utils::FilePath &workingDirectory, const Utils::Environment &environment);
    ~VcsCommand() override;

    void setDisplayName(const QString &name);

    void addJob(const Utils::CommandLine &command, int timeoutS,
                const Utils::FilePath &workingDirectory = {},
                const ExitCodeInterpreter &interpreter = {});
    void start();

    void addFlags(RunFlags f);

    void setEncoding(const Utils::TextEncoding &encoding);

    void setProgressParser(const Core::ProgressParser &parser);

    void setStdOutCallback(const Utils::TextChannelCallback &callback);
    void setStdErrCallback(const Utils::TextChannelCallback &callback);

    static CommandResult runBlocking(const Utils::FilePath &workingDirectory,
                                     const Utils::Environment &environment,
                                     const Utils::CommandLine &command,
                                     RunFlags flags,
                                     int timeoutS,
                                     const Utils::TextEncoding &codec);
    void cancel();

    QString cleanedStdOut() const;
    QString cleanedStdErr() const;
    Utils::ProcessResult result() const;

signals:
    void done();

private:
    CommandResult runBlockingHelper(const Utils::CommandLine &command, int timeoutS);

    class Internal::VcsCommandPrivate *const d;
};

} // namespace Utils
