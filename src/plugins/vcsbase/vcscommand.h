// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"
#include "vcsenums.h"

#include <coreplugin/progressmanager/processprogress.h>

#include <utils/filepath.h>
#include <utils/processenums.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {
class CommandLine;
class Environment;
class Process;
}

namespace VcsBase {

namespace Internal { class VcsCommandPrivate; }

class VcsCommand;

class VCSBASE_EXPORT CommandResult
{
public:
    CommandResult() = default;
    CommandResult(const Utils::Process &process);
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

class VCSBASE_EXPORT VcsCommand final : public QObject
{
    Q_OBJECT

public:
    VcsCommand(const Utils::FilePath &workingDirectory, const Utils::Environment &environment);
    ~VcsCommand() override;

    void setDisplayName(const QString &name);

    void addJob(const Utils::CommandLine &command, int timeoutS,
                const Utils::FilePath &workingDirectory = {},
                const Utils::ExitCodeInterpreter &interpreter = {});
    void start();

    void addFlags(RunFlags f);

    void setCodec(QTextCodec *codec);

    void setProgressParser(const Core::ProgressParser &parser);

    static CommandResult runBlocking(const Utils::FilePath &workingDirectory,
                                     const Utils::Environment &environmentconst,
                                     const Utils::CommandLine &command, RunFlags flags,
                                     int timeoutS, QTextCodec *codec);
    void cancel();

    QString cleanedStdOut() const;
    QString cleanedStdErr() const;
    Utils::ProcessResult result() const;

signals:
    void stdOutText(const QString &);
    void stdErrText(const QString &);
    void done();

private:
    CommandResult runBlockingHelper(const Utils::CommandLine &command, int timeoutS);

    class Internal::VcsCommandPrivate *const d;
};

} // namespace Utils
