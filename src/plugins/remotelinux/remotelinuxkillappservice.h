/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "abstractremotelinuxdeployservice.h"

namespace RemoteLinux {
namespace Internal { class RemoteLinuxKillAppServicePrivate; }

class REMOTELINUX_EXPORT RemoteLinuxKillAppService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
public:
    RemoteLinuxKillAppService();
    ~RemoteLinuxKillAppService() override;

    void setRemoteExecutable(const QString &filePath);

private:
    void handleStdErr();
    void handleProcessFinished();

    bool isDeploymentNecessary() const override;
    void doDeviceSetup() override { handleDeviceSetupDone(true); }
    void stopDeviceSetup() override { handleDeviceSetupDone(false); }

    void doDeploy() override;
    void stopDeployment() override;

    void handleSignalOpFinished(const QString &errorMessage);
    void cleanup();
    void finishDeployment();

    Internal::RemoteLinuxKillAppServicePrivate * const d;
};

} // namespace RemoteLinux
