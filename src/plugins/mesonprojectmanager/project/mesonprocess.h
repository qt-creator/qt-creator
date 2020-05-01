/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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
#include <exewrappers/mesonwrapper.h>

#include <utils/qtcprocess.h>

#include <QBuffer>
#include <QByteArray>
#include <QElapsedTimer>
#include <QFutureInterface>
#include <QObject>
#include <QProcess>
#include <QTimer>

#include <memory>

namespace MesonProjectManager {
namespace Internal {
class MesonProcess final : public QObject
{
    Q_OBJECT
public:
    MesonProcess();
    bool run(const Command &command,
             const Utils::Environment env,
             const QString &projectName,
             bool captureStdo = false);

    QProcess::ProcessState state() const;

    // Update progress information:
    void reportCanceled();
    void reportFinished();
    void setProgressValue(int p);

    const QByteArray &stdOut() const { return m_stdo; }
    const QByteArray &stdErr() const { return m_stderr; }
signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);
    void readyReadStandardOutput(const QByteArray &data);

private:
    void handleProcessFinished(int code, QProcess::ExitStatus status);
    void handleProcessError(QProcess::ProcessError error);
    void checkForCancelled();
    void setupProcess(const Command &command, const Utils::Environment env, bool captureStdo);

    bool sanityCheck(const Command &command) const;

    void processStandardOutput();
    void processStandardError();

    std::unique_ptr<Utils::QtcProcess> m_process;
    QFutureInterface<void> m_future;
    bool m_processWasCanceled = false;
    QTimer m_cancelTimer;
    QElapsedTimer m_elapsed;
    QByteArray m_stdo;
    QByteArray m_stderr;
    Command m_currentCommand;
};
} // namespace Internal
} // namespace MesonProjectManager
