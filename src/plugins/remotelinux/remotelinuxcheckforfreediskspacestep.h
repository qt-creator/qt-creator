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

#include "abstractremotelinuxdeploystep.h"

namespace RemoteLinux {
namespace Internal { class RemoteLinuxCheckForFreeDiskSpaceStepPrivate; }

class REMOTELINUX_EXPORT RemoteLinuxCheckForFreeDiskSpaceStep : public AbstractRemoteLinuxDeployStep
{
    Q_OBJECT
public:
    explicit RemoteLinuxCheckForFreeDiskSpaceStep(ProjectExplorer::BuildStepList *bsl,
            Core::Id id = stepId());
    RemoteLinuxCheckForFreeDiskSpaceStep(ProjectExplorer::BuildStepList *bsl,
            RemoteLinuxCheckForFreeDiskSpaceStep *other);
    ~RemoteLinuxCheckForFreeDiskSpaceStep() override;

    void setPathToCheck(const QString &path);
    QString pathToCheck() const;

    void setRequiredSpaceInBytes(quint64 space);
    quint64 requiredSpaceInBytes() const;

    static Core::Id stepId();
    static QString stepDisplayName();

protected:
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    bool initInternal(QString *error) override;
    AbstractRemoteLinuxDeployService *deployService() const override;

private:
    void ctor();

    Internal::RemoteLinuxCheckForFreeDiskSpaceStepPrivate *d;
};

} // namespace RemoteLinux
