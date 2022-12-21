// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QByteArray>
#include <QElapsedTimer>
#include <QObject>
#include <QProcess>

#include <memory>

namespace Utils {
class Environment;
class QtcProcess;
}

namespace MesonProjectManager {
namespace Internal {

class Command;

class MesonProcess final : public QObject
{
    Q_OBJECT
public:
    MesonProcess();
    ~MesonProcess();
    bool run(const Command &command, const Utils::Environment &env,
             const QString &projectName, bool captureStdo = false);

    const QByteArray &stdOut() const { return m_stdo; }
    const QByteArray &stdErr() const { return m_stderr; }
signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void readyReadStandardOutput(const QByteArray &data);

private:
    void handleProcessDone();
    void setupProcess(const Command &command, const Utils::Environment &env,
                      const QString &projectName, bool captureStdo);
    bool sanityCheck(const Command &command) const;

    void processStandardOutput();
    void processStandardError();

    std::unique_ptr<Utils::QtcProcess> m_process;
    QElapsedTimer m_elapsed;
    QByteArray m_stdo;
    QByteArray m_stderr;
};

} // namespace Internal
} // namespace MesonProjectManager
