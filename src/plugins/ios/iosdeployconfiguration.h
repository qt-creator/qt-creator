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
#ifndef IOSDEPLOYCONFIGURATION_H
#define IOSDEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>
#include <utils/qtcoverride.h>

namespace Ios {
namespace Internal {

const char IOS_DEPLOYCONFIGURATION_ID[] = "Qt4ProjectManager.IosDeployConfiguration";
const char IOS_DC_PREFIX[] = "Qt4ProjectManager.IosDeployConfiguration.";

class IosDeployConfiguration : public ProjectExplorer::DeployConfiguration
{
    Q_OBJECT
    friend class IosDeployConfigurationFactory;

public:
    IosDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id);

protected:
    IosDeployConfiguration(ProjectExplorer::Target *parent,
                           ProjectExplorer::DeployConfiguration *source);

};

class IosDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_OBJECT

public:
    explicit IosDeployConfigurationFactory(QObject *parent = 0);

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const QTC_OVERRIDE;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, Core::Id id) QTC_OVERRIDE;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const QTC_OVERRIDE;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent,
                                                  const QVariantMap &map) QTC_OVERRIDE;
    bool canClone(ProjectExplorer::Target *parent,
                  ProjectExplorer::DeployConfiguration *source) const QTC_OVERRIDE;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *source) QTC_OVERRIDE;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const QTC_OVERRIDE;
    // used to translate the ids to names to display to the user
    QString displayNameForId(Core::Id id) const QTC_OVERRIDE;

private:
    bool canHandle(ProjectExplorer::Target *parent) const QTC_OVERRIDE;
};

} // namespace Internal
} // namespace Ios

#endif // IOSDEPLOYCONFIGURATION_H
