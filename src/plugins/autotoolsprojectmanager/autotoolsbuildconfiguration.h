/**************************************************************************
**
** Copyright (C) 2013 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#ifndef AUTOTOOLSBUILDCONFIGURATION_H
#define AUTOTOOLSBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>

namespace AutotoolsProjectManager {
namespace Internal {

class AutotoolsTarget;
class AutotoolsBuildConfigurationFactory;

class AutotoolsBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class AutotoolsBuildConfigurationFactory;

public:
    explicit AutotoolsBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();

    QString buildDirectory() const;
    void setBuildDirectory(const QString &buildDirectory);
    QVariantMap toMap() const;
    BuildType buildType() const;

protected:
    AutotoolsBuildConfiguration(ProjectExplorer::Target *parent, const Core::Id id);
    AutotoolsBuildConfiguration(ProjectExplorer::Target *parent, AutotoolsBuildConfiguration *source);

    bool fromMap(const QVariantMap &map);

private:
    QString m_buildDirectory;
};

class AutotoolsBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit AutotoolsBuildConfigurationFactory(QObject *parent = 0);

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    AutotoolsBuildConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name = QString());
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    AutotoolsBuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    AutotoolsBuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

    static AutotoolsBuildConfiguration *createDefaultConfiguration(ProjectExplorer::Target *target);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

} // namespace Internal
} // namespace AutotoolsProjectManager
#endif // AUTOTOOLSBUILDCONFIGURATION_H
