/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGER_INTERNAL_BUILDCONFIGURATION_H
#define VCPROJECTMANAGER_INTERNAL_BUILDCONFIGURATION_H

#include "vcproject.h"
#include "vcprojectmodel/configuration.h"

#include <projectexplorer/buildconfiguration.h>

namespace VcProjectManager {
namespace Internal {

class VcProjectBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class VcProjectBuildConfigurationFactory;

public:
    explicit VcProjectBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();
    QString buildDirectory() const;
    ProjectExplorer::IOutputParser *createOutputParser() const;
    BuildType buildType() const;

    void setConfiguration(Configuration::Ptr config);
    QString configurationNameOnly() const;
    QString platformNameOnly() const;

    QVariantMap toMap() const;

private slots:
    void reloadConfigurationName();

protected:
    VcProjectBuildConfiguration(ProjectExplorer::Target *parent, VcProjectBuildConfiguration *source);
    bool fromMap(const QVariantMap &map);

private:
    QString m_buildDirectory;
    Configuration::Ptr m_configuration;
};

class VcProjectBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit VcProjectBuildConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;
    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    VcProjectBuildConfiguration* create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    VcProjectBuildConfiguration* restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    VcProjectBuildConfiguration* clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

} // namespace Internal
} // namespace VcProjectManager
#endif // VCPROJECTMANAGER_INTERNAL_BUILDCONFIGURATION_H
