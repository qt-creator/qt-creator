/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnx_export.h"
#include <remotelinux/linuxdevice.h>
#include <projectexplorer/devicesupport/sshdeviceprocess.h>

namespace Qnx {
namespace Internal {

class QnxDeviceProcess : public ProjectExplorer::SshDeviceProcess
{
public:
    QnxDeviceProcess(const QSharedPointer<const ProjectExplorer::IDevice> &device, QObject *parent);

    void interrupt() { doSignal(2); }
    void terminate() { doSignal(15); }
    void kill() { doSignal(9); }
    QString fullCommandLine(const ProjectExplorer::StandardRunnable &runnable) const;

private:
    void doSignal(int sig);
    QString m_pidFile;
};

} // namespace Internal
} // namespace Qnx
