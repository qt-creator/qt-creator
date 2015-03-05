/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REMOTELINUXSIGNALOPERATION_H
#define REMOTELINUXSIGNALOPERATION_H

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>
#include <ssh/sshconnection.h>

namespace QSsh {
class SshRemoteProcessRunner;
}

namespace RemoteLinux {

class REMOTELINUX_EXPORT RemoteLinuxSignalOperation
        : public ProjectExplorer::DeviceProcessSignalOperation
{
    Q_OBJECT
public:
    virtual ~RemoteLinuxSignalOperation();

    void killProcess(int pid);
    void killProcess(const QString &filePath);
    void interruptProcess(int pid);
    void interruptProcess(const QString &filePath);

protected:
    RemoteLinuxSignalOperation(const QSsh::SshConnectionParameters &sshParameters);

private slots:
    void runnerProcessFinished();
    void runnerConnectionError();

private:
    virtual QString killProcessByNameCommandLine(const QString &filePath) const;
    virtual QString interruptProcessByNameCommandLine(const QString &filePath) const;
    void run(const QString &command);
    void finish();

    const QSsh::SshConnectionParameters m_sshParameters;
    QSsh::SshRemoteProcessRunner *m_runner;

    friend class LinuxDevice;
};

}

#endif // REMOTELINUXSIGNALOPERATION_H
