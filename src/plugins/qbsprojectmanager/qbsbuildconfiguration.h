// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QbsBuildConfiguration(ProjectExplorer::Target *target, Utils::Id id);
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

    QString equivalentCommandLine(const QbsBuildStepData &stepData) const;

    Utils::StringAspect configurationName{this};
    ProjectExplorer::SeparateDebugInfoAspect separateDebugInfoSetting{this};
    QtSupport::QmlDebuggingAspect qmlDebuggingSetting{this};
    QtSupport::QtQuickCompilerAspect qtQuickCompilerSetting{this};

signals:
    void qbsConfigurationChanged();

private:
    void fromMap(const Utils::Store &map) override;
    void restrictNextBuild(const ProjectExplorer::RunConfiguration *rc) override;
    void triggerReparseIfActive();

    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;
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
