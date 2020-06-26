/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/makestep.h>

namespace ProjectExplorer { class BaseStringAspect; }
namespace Utils { class FilePath; }

namespace RemoteLinux {

class REMOTELINUX_EXPORT MakeInstallStep : public ProjectExplorer::MakeStep
{
    Q_OBJECT
public:
    MakeInstallStep(ProjectExplorer::BuildStepList *parent, Utils::Id id);

    static Utils::Id stepId();
    static QString displayName();

private:
    bool fromMap(const QVariantMap &map) override;
    ProjectExplorer::BuildStepConfigWidget * createConfigWidget() override;
    bool init() override;
    void finish(bool success) override;
    void stdError(const QString &line) override;
    bool isJobCountSupported() const override { return false; }

    Utils::FilePath installRoot() const;
    bool cleanInstallRoot() const;

    void updateCommandFromAspect();
    void updateArgsFromAspect();
    void updateFullCommandLine();
    void updateFromCustomCommandLineAspect();

    ProjectExplorer::BaseStringAspect *customCommandLineAspect() const;

    ProjectExplorer::DeploymentData m_deploymentData;
    bool m_noInstallTarget = false;
    bool m_isCmakeProject = false;
};

} // namespace RemoteLinux
