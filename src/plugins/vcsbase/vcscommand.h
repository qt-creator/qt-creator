// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"
#include "vcsenums.h"

#include <coreplugin/progressmanager/processprogress.h>

#include <QtTaskTree/QTaskTree>

#include <utils/filepath.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/textcodec.h>

#include <QObject>

namespace VcsBase {

namespace Internal { class VcsCommandPrivate; }

using ExitCodeInterpreter = std::function<Utils::ProcessResult(int /*exitCode*/)>;

class VCSBASE_EXPORT CommandResult
{
public:
    CommandResult() = default;
    CommandResult(const Utils::Process &process);
    CommandResult(const Utils::Process &process, Utils::ProcessResult result);
    CommandResult(Utils::ProcessResult result, const QString &exitMessage)
        : m_result(result), m_exitMessage(exitMessage) {}

    Utils::ProcessResult result() const { return m_result; }
    int exitCode() const { return m_exitCode; }
    QString exitMessage() const { return m_exitMessage; }

    QString cleanedStdOut() const { return m_cleanedStdOut; }
    QString cleanedStdErr() const { return m_cleanedStdErr; }

    QByteArray rawStdOut() const { return m_rawStdOut; }

    Utils::FilePath workingDirectory() const { return m_workingDirectory; }

private:
    Utils::ProcessResult m_result = Utils::ProcessResult::StartFailed;
    int m_exitCode = 0;
    QString m_exitMessage;

    QString m_cleanedStdOut;
    QString m_cleanedStdErr;

    QByteArray m_rawStdOut;

    Utils::FilePath m_workingDirectory;
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

VCSBASE_EXPORT QtTaskTree::ExecutableItem errorTask(const Utils::FilePath &workingDir,
                                                 const QString &errorMessage);

VCSBASE_EXPORT Utils::ProcessTask vcsProcessTask(const VcsProcessData &data,
    const std::optional<QtTaskTree::Storage<CommandResult>> &resultStorage = {});

// TODO: Avoid, migrate to asynchronous task tree recipes.
VCSBASE_EXPORT CommandResult vcsRunBlocking(const VcsProcessData &data,
    const std::chrono::seconds timeout = std::chrono::seconds(10));

} // namespace Utils
