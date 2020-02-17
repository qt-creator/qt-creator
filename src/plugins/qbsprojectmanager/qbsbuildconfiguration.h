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

#include "qbsprojectmanager_global.h"

#include "qbsproject.h"

#include <projectexplorer/buildaspects.h>
#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtbuildaspects.h>

namespace ProjectExplorer { class BuildStep; }

namespace QbsProjectManager {
namespace Internal {

class QbsBuildStep;

class QbsBuildStepData
{
public:
    QString command;
    bool dryRun = false;
    bool keepGoing = false;
    bool forceProbeExecution = false;
    bool showCommandLines = false;
    bool noInstall = false;
    bool noBuild = false;
    bool cleanInstallRoot = false;
    bool isInstallStep = false;
    int jobCount = 0;
    Utils::FilePath installRoot;
};

class QbsBuildConfiguration final : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    QbsBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    ~QbsBuildConfiguration() final;

public:
    ProjectExplorer::BuildSystem *buildSystem() const final;

    QbsBuildStep *qbsStep() const;
    QVariantMap qbsConfiguration() const;

    BuildType buildType() const override;

    void setChangedFiles(const QStringList &files);
    QStringList changedFiles() const;

    void setActiveFileTags(const QStringList &fileTags);
    QStringList activeFileTags() const;

    void setProducts(const QStringList &products);
    QStringList products() const;

    QString configurationName() const;
    QString equivalentCommandLine(const QbsBuildStepData &stepData) const;

    ProjectExplorer::TriState qmlDebuggingSetting() const;
    ProjectExplorer::TriState qtQuickCompilerSetting() const;
    ProjectExplorer::TriState separateDebugInfoSetting() const;

signals:
    void qbsConfigurationChanged();

private:
    bool fromMap(const QVariantMap &map) override;
    void restrictNextBuild(const ProjectExplorer::RunConfiguration *rc) override;
    void triggerReparseIfActive();

    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;
    ProjectExplorer::BaseStringAspect *m_configurationName = nullptr;
    QbsBuildSystem *m_buildSystem = nullptr;
};

class QbsBuildConfigurationFactory final : public ProjectExplorer::BuildConfigurationFactory
{
public:
    QbsBuildConfigurationFactory();

private:
    ProjectExplorer::BuildInfo createBuildInfo(ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace Internal
} // namespace QbsProjectManager
