// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "mesonwrapper.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QFutureInterface>
#include <QObject>
#include <QProcess>
#include <QTimer>

#include <memory>

namespace Utils { class QtcProcess; }

namespace MesonProjectManager {
namespace Internal {

class MesonProcess final : public QObject
{
    Q_OBJECT
public:
    MesonProcess();
    bool run(const Command &command,
             const Utils::Environment &env,
             const QString &projectName,
             bool captureStdo = false);

    const QByteArray &stdOut() const { return m_stdo; }
    const QByteArray &stdErr() const { return m_stderr; }
signals:
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void readyReadStandardOutput(const QByteArray &data);

private:
    void handleProcessDone();
    void checkForCancelled();
    void setupProcess(const Command &command, const Utils::Environment env, bool captureStdo);

    bool sanityCheck(const Command &command) const;

    void processStandardOutput();
    void processStandardError();

    std::unique_ptr<Utils::QtcProcess> m_process;
    QFutureInterface<void> m_future;
    QTimer m_cancelTimer;
    QElapsedTimer m_elapsed;
    QByteArray m_stdo;
    QByteArray m_stderr;
};

} // namespace Internal
} // namespace MesonProjectManager
