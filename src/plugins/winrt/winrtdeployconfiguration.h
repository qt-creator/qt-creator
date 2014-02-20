/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef WINRTDEPLOYCONFIGURATION_H
#define WINRTDEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/buildstep.h>

namespace WinRt {
namespace Internal {

class WinRtDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
public:
    explicit WinRtDeployConfigurationFactory(QObject *parent = 0);
    QString displayNameForId(const Core::Id id) const;
    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const;
    bool canCreate(ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::DeployConfiguration *source) const;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *source);
};

class WinRtDeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
public:
    QList<Core::Id> availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const Core::Id id) const;
    bool canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const Core::Id id);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
                                      ProjectExplorer::BuildStep *source);
};

class WinRtDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
private:
    friend class WinRtDeployConfigurationFactory;
    explicit WinRtDeployConfiguration(ProjectExplorer::Target *target, Core::Id id);
    explicit WinRtDeployConfiguration(ProjectExplorer::Target *target,
                                      ProjectExplorer::DeployConfiguration *source);
};

} // namespace Internal
} // namespace WinRt

#endif // WINRTDEPLOYCONFIGURATION_H
