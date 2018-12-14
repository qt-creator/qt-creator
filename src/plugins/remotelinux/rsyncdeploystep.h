/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "abstractremotelinuxdeploystep.h"
#include "remotelinux_export.h"

namespace QSsh { class SshConnection; }

namespace RemoteLinux {

class RsyncCommandLine
{
public:
    RsyncCommandLine(const QStringList &o, const QString &h) : options(o), remoteHostSpec(h) {}
    const QStringList options;
    const QString remoteHostSpec;
};

class REMOTELINUX_EXPORT RsyncDeployStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    explicit RsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    ~RsyncDeployStep() override;

    static Core::Id stepId();
    static QString displayName();

    static RsyncCommandLine rsyncCommand(const QSsh::SshConnection &sshConnection);

private:
    AbstractRemoteLinuxDeployService *deployService() const override;

    bool initInternal(QString *error = nullptr) override;

    class RsyncDeployStepPrivate;
    RsyncDeployStepPrivate * const d;
};

} // namespace RemoteLinux
