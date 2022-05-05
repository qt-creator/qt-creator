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

#include "utils_global.h"

#include <QProcess>
#include <QThread>

#include <memory>

namespace Utils {

class CommandLine;
class QtcProcess;

class DeviceShellImpl;

class QTCREATOR_UTILS_EXPORT DeviceShell : public QObject
{
    Q_OBJECT

public:
    struct RunResult
    {
        int exitCode;
        QByteArray stdOutput;
    };

    DeviceShell();
    virtual ~DeviceShell();

    bool runInShell(const CommandLine &cmd, const QByteArray &stdInData = {});
    RunResult outputForRunInShell(const CommandLine &cmd, const QByteArray &stdInData = {});

    bool waitForStarted();

signals:
    void done();
    void errorOccurred(QProcess::ProcessError error);

protected:
    bool runInShellImpl(const CommandLine &cmd, const QByteArray &stdInData = {});
    RunResult outputForRunInShellImpl(const CommandLine &cmd, const QByteArray &stdInData = {});

    bool start();
    void close();

private:
    virtual void setupShellProcess(QtcProcess *shellProcess);
    virtual void startupFailed(const CommandLine &cmdLine);

    void closeShellProcess();

private:
    QtcProcess *m_shellProcess;
    QThread m_thread;
};

} // namespace Utils
