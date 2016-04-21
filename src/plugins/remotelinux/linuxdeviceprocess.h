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

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/sshdeviceprocess.h>

#include <QStringList>

namespace RemoteLinux {

class REMOTELINUX_EXPORT LinuxDeviceProcess : public ProjectExplorer::SshDeviceProcess
{
    Q_OBJECT
public:
    explicit LinuxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device,
                                QObject *parent = 0);

    // Files to source before executing the command (if they exist). Overrides the default.
    void setRcFilesToSource(const QStringList &filePaths);

    QByteArray readAllStandardOutput() override;

private:
    QString fullCommandLine(const ProjectExplorer::StandardRunnable &) const override;
    qint64 processId() const override;

    QStringList rcFilesToSource() const;

    QStringList m_rcFilesToSource;
    QByteArray m_processIdString;
    qint64 m_processId;
};

} // namespace RemoteLinux
