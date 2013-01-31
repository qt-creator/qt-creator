/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef GENERICBUILDCONFIGURATION_H
#define GENERICBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>

namespace Utils { class PathChooser; }

namespace GenericProjectManager {
namespace Internal {

class GenericTarget;
class GenericBuildConfigurationFactory;

class GenericBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
    friend class GenericBuildConfigurationFactory;

public:
    explicit GenericBuildConfiguration(ProjectExplorer::Target *parent);

    ProjectExplorer::NamedWidget *createConfigWidget();
    QString buildDirectory() const;

    QString rawBuildDirectory() const;
    void setBuildDirectory(const QString &buildDirectory);

    QVariantMap toMap() const;
    BuildType buildType() const;

protected:
    GenericBuildConfiguration(ProjectExplorer::Target *parent, GenericBuildConfiguration *source);
    GenericBuildConfiguration(ProjectExplorer::Target *parent, const Core::Id id);
    virtual bool fromMap(const QVariantMap &map);

private:
    QString m_buildDirectory;
};

class GenericBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit GenericBuildConfigurationFactory(QObject *parent = 0);
    ~GenericBuildConfigurationFactory();

    QList<Core::Id> availableCreationIds(const ProjectExplorer::Target *parent) const;
    QString displayNameForId(const Core::Id id) const;

    bool canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const;
    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name = QString());
    bool canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source);
    bool canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);

private:
    bool canHandle(const ProjectExplorer::Target *t) const;
};

class GenericBuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    GenericBuildSettingsWidget(GenericBuildConfiguration *bc);

private slots:
    void buildDirectoryChanged();

private:
    Utils::PathChooser *m_pathChooser;
    GenericBuildConfiguration *m_buildConfiguration;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICBUILDCONFIGURATION_H
