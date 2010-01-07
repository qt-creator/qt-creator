/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CMAKEBUILDCONFIGURATION_H
#define CMAKEBUILDCONFIGURATION_H

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/toolchain.h>

namespace CMakeProjectManager {
namespace Internal {

class CMakeProject;

class CMakeBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT
public:
    CMakeBuildConfiguration(CMakeProject *pro);
    CMakeBuildConfiguration(CMakeProject *pro, const QMap<QString, QVariant> &map);
    CMakeBuildConfiguration(CMakeBuildConfiguration *source);
    ~CMakeBuildConfiguration();

    CMakeProject *cmakeProject() const;

    ProjectExplorer::Environment environment() const;
    ProjectExplorer::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;
    void setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff);
    QList<ProjectExplorer::EnvironmentItem> userEnvironmentChanges() const;
    bool useSystemEnvironment() const;
    void setUseSystemEnvironment(bool b);

    virtual QString buildDirectory() const;

    ProjectExplorer::ToolChain::ToolChainType toolChainType() const;
    ProjectExplorer::ToolChain *toolChain() const;


    void setBuildDirectory(const QString &buildDirectory);

    QString msvcVersion() const;
    void setMsvcVersion(const QString &msvcVersion);

    void toMap(QMap<QString, QVariant> &map) const;

signals:
    void msvcVersionChanged();

private:
    void updateToolChain() const;
    mutable ProjectExplorer::ToolChain *m_toolChain;
    bool m_clearSystemEnvironment;
    QList<ProjectExplorer::EnvironmentItem> m_userEnvironmentChanges;
    QString m_buildDirectory;
    QString m_msvcVersion;
};

class CMakeBuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    CMakeBuildConfigurationFactory(CMakeProject *project);
    ~CMakeBuildConfigurationFactory();

    QStringList availableCreationIds() const;
    QString displayNameForId(const QString &id) const;

    ProjectExplorer::BuildConfiguration *create(const QString &id) const;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::BuildConfiguration *source) const;
    ProjectExplorer::BuildConfiguration *restore(const QVariantMap &map) const;

private:
    CMakeProject *m_project;
};

} // namespace Internal
} // namespace CMakeProjectManager

#endif // CMAKEBUILDCONFIGURATION_H
