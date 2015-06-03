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
#ifndef IOSDEPLOYCONFIGURATION_H
#define IOSDEPLOYCONFIGURATION_H

#include <projectexplorer/deployconfiguration.h>

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

    bool canCreate(ProjectExplorer::Target *parent, Core::Id id) const override;
    ProjectExplorer::DeployConfiguration *create(ProjectExplorer::Target *parent, Core::Id id) override;
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const override;
    ProjectExplorer::DeployConfiguration *restore(ProjectExplorer::Target *parent,
                                                  const QVariantMap &map) override;
    bool canClone(ProjectExplorer::Target *parent,
                  ProjectExplorer::DeployConfiguration *source) const override;
    ProjectExplorer::DeployConfiguration *clone(ProjectExplorer::Target *parent,
                                                ProjectExplorer::DeployConfiguration *source) override;

    QList<Core::Id> availableCreationIds(ProjectExplorer::Target *parent) const override;
    // used to translate the ids to names to display to the user
    QString displayNameForId(Core::Id id) const override;
};

} // namespace Internal
} // namespace Ios

#endif // IOSDEPLOYCONFIGURATION_H
