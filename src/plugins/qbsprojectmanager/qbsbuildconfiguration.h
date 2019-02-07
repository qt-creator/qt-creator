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

#include <projectexplorer/buildconfiguration.h>
#include <qtsupport/baseqtversion.h>

namespace ProjectExplorer { class BuildStep; }

namespace QbsProjectManager {
namespace Internal {

class QbsBuildConfigurationWidget;
class QbsBuildStep;
class QbsProject;

class QbsBuildConfiguration : public ProjectExplorer::BuildConfiguration
{
    Q_OBJECT

    friend class ProjectExplorer::BuildConfigurationFactory;
    QbsBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);

public:
    void initialize(const ProjectExplorer::BuildInfo &info) override;
    ProjectExplorer::NamedWidget *createConfigWidget() override;

    QbsBuildStep *qbsStep() const;
    QVariantMap qbsConfiguration() const;

    Internal::QbsProject *project() const override;

    bool isEnabled() const override;
    QString disabledReason() const override;

    BuildType buildType() const override;

    void setChangedFiles(const QStringList &files);
    QStringList changedFiles() const;

    void setActiveFileTags(const QStringList &fileTags);
    QStringList activeFileTags() const;

    void setProducts(const QStringList &products);
    QStringList products() const;

    void emitBuildTypeChanged();

    void setConfigurationName(const QString &configName);
    QString configurationName() const;

    QString equivalentCommandLine(const ProjectExplorer::BuildStep *buildStep) const;

signals:
    void qbsConfigurationChanged();

private:
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    bool m_isParsing = true;
    bool m_parsingError = false;
    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;
    QString m_configurationName;

    friend class QbsBuildConfigurationWidget;
};

class QbsBuildConfigurationFactory : public ProjectExplorer::BuildConfigurationFactory
{
    Q_OBJECT

public:
    QbsBuildConfigurationFactory();

    QList<ProjectExplorer::BuildInfo> availableBuilds(const ProjectExplorer::Target *parent) const override;
    QList<ProjectExplorer::BuildInfo> availableSetups(const ProjectExplorer::Kit *k,
                                                      const QString &projectPath) const override;

private:
    ProjectExplorer::BuildInfo createBuildInfo(const ProjectExplorer::Kit *k,
                                                ProjectExplorer::BuildConfiguration::BuildType type) const;
};

} // namespace Internal
} // namespace QbsProjectManager
