/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "projectexplorer_export.h"

#include "devicesupport/idevice.h"

#include <utils/outputformat.h>
#include <utils/processhandle.h>

#include <QProcess>

namespace Utils { class ProcessHandle; }

namespace ProjectExplorer {

class Runnable;
namespace Internal { class ApplicationLauncherPrivate; }

// Documentation inside.
class PROJECTEXPLORER_EXPORT ApplicationLauncher : public QObject
{
    Q_OBJECT

public:
    enum Mode {
        Console,
        Gui
    };

    explicit ApplicationLauncher(QObject *parent = nullptr);
    ~ApplicationLauncher() override;

    void setProcessChannelMode(QProcess::ProcessChannelMode mode);
    void start(const Runnable &runnable);
    void start(const Runnable &runnable, const IDevice::ConstPtr &device);
    void stop();
    bool isRunning() const;
    Utils::ProcessHandle applicationPID() const;

    QString errorString() const;
    QProcess::ProcessError processError() const;
    void bringToForeground();

    static QString msgWinCannotRetrieveDebuggingOutput();

signals:
    void appendMessage(const QString &message, Utils::OutputFormat format);
    void processStarted();
    void processExited(int exitCode, QProcess::ExitStatus);
    void error(QProcess::ProcessError error);

    void remoteStdout(const QByteArray &output);
    void remoteStderr(const QByteArray &output);
    void reportProgress(const QString &progressOutput);
    void reportError(const QString &errorOutput);
    void remoteProcessStarted();
    void finished(bool success);

private:
    Internal::ApplicationLauncherPrivate *d;
};

} // namespace ProjectExplorer
