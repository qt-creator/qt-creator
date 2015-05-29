//
// Copyright (C) 2013 Mateusz Łoskot <mateusz@loskot.net>
//
// This file is part of Qt Creator Boost.Build plugin project.
//
// This is free software; you can redistribute and/or modify it under
// the terms of the GNU Lesser General Public License, Version 2.1
// as published by the Free Software Foundation.
// See accompanying file LICENSE.txt or copy at
// http://www.gnu.org/licenses/lgpl-2.1-standalone.html.
//
#ifndef BBBUILDCONFIGURATION_HPP
#define BBBUILDCONFIGURATION_HPP

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/namedwidget.h>
#include <utils/fileutils.h>

#include <QList>
#include <QString>
#include <QVariantMap>

namespace Utils {
class FileName;
class PathChooser;
}

namespace BoostBuildProjectManager {
namespace Internal {

class BuildInfo;
class BuildSettingsWidget;

class BuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class BuildConfigurationFactory;

public:
    explicit BuildConfiguration(ProjectExplorer::Target* parent);

    QVariantMap toMap() const;

    ProjectExplorer::NamedWidget* createConfigWidget();

    BuildType buildType() const;

    Utils::FileName workingDirectory() const;
    void setWorkingDirectory(Utils::FileName const& dir);

signals:
    void workingDirectoryChanged();

protected:
    BuildConfiguration(ProjectExplorer::Target* parent, BuildConfiguration* source);
    BuildConfiguration(ProjectExplorer::Target* parent, Core::Id const id);

    bool fromMap(QVariantMap const& map);

    friend class BuildSettingsWidget;

private slots:
    void emitWorkingDirectoryChanged();

private:
    Utils::FileName m_workingDirectory;
    Utils::FileName m_lastEmmitedWorkingDirectory;
};

class BuildConfigurationFactory : public ProjectExplorer::IBuildConfigurationFactory
{
    Q_OBJECT

public:
    explicit BuildConfigurationFactory(QObject* parent = 0);

    int priority(ProjectExplorer::Target const* parent) const;
    int priority(ProjectExplorer::Kit const* k, QString const& projectPath) const;

    QList<ProjectExplorer::BuildInfo*>
    availableBuilds(ProjectExplorer::Target const* parent) const;

    QList<ProjectExplorer::BuildInfo*>
    availableSetups(ProjectExplorer::Kit const* k, QString const& projectPath) const;

    ProjectExplorer::BuildConfiguration*
    create(ProjectExplorer::Target* parent
         , ProjectExplorer::BuildInfo const* info) const;

    bool
    canClone(ProjectExplorer::Target const* parent
           , ProjectExplorer::BuildConfiguration* source) const;

    BuildConfiguration*
    clone(ProjectExplorer::Target* parent, ProjectExplorer::BuildConfiguration* source);

    bool
    canRestore(ProjectExplorer::Target const* parent, QVariantMap const& map) const;

    BuildConfiguration*
    restore(ProjectExplorer::Target *parent, QVariantMap const& map);

    static Utils::FileName defaultBuildDirectory(QString const& top);
    static Utils::FileName defaultWorkingDirectory(QString const& top);

private:
    bool canHandle(ProjectExplorer::Target const* target) const;

    BuildInfo*
    createBuildInfo(ProjectExplorer::Kit const* k
                  , QString const& projectPath
                  , BuildConfiguration::BuildType type) const;
};

class BuildSettingsWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    BuildSettingsWidget(BuildConfiguration* bc);

private slots:
    void environmentHasChanged();
    void buildDirectoryChanged();
    void workingDirectoryChanged();

private:
    BuildConfiguration *m_bc;
    Utils::PathChooser *m_workPathChooser;
    Utils::PathChooser *m_buildPathChooser;
};

} // namespace Internal
} // namespace BoostBuildProjectManager

#endif // BBBUILDCONFIGURATION_HPP
