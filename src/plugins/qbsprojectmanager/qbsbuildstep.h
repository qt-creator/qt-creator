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

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsProject;
class QbsSession;

class ArchitecturesAspect;
class QbsBuildStepConfigWidget;

class QbsBuildStep final : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    enum VariableHandling
    {
        PreserveVariables,
        ExpandVariables
    };

    QbsBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);
    ~QbsBuildStep() override;

    QVariantMap qbsConfiguration(VariableHandling variableHandling) const;
    void setQbsConfiguration(const QVariantMap &config);

    bool keepGoing() const { return m_keepGoing->value(); }
    bool showCommandLines() const { return m_showCommandLines->value(); }
    bool install() const { return m_install->value(); }
    bool cleanInstallRoot() const { return m_cleanInstallDir->value(); }
    bool hasCustomInstallRoot() const;
    Utils::FilePath installRoot(VariableHandling variableHandling = ExpandVariables) const;
    int maxJobs() const;
    QString buildVariant() const;

    bool forceProbes() const { return m_forceProbes->value(); }

    QbsBuildSystem *qbsBuildSystem() const;
    QbsBuildStepData stepData() const;

    void dropSession();

signals:
    void qbsConfigurationChanged();
    void qbsBuildOptionsChanged();

private:
    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() override;
    void doCancel() override;
    QWidget *createConfigWidget() override;
    bool fromMap(const QVariantMap &map) override;
    QVariantMap toMap() const override;

    void buildingDone(const ErrorInfo &error);
    void reparsingDone(bool success);
    void handleTaskStarted(const QString &desciption, int max);
    void handleProgress(int value);
    void handleCommandDescription(const QString &message);
    void handleProcessResult(
            const Utils::FilePath &executable,
            const QStringList &arguments,
            const Utils::FilePath &workingDir,
            const QStringList &stdOut,
            const QStringList &stdErr,
            bool success);

    void createTaskAndOutput(ProjectExplorer::Task::TaskType type,
                             const QString &message, const QString &file, int line);

    void setBuildVariant(const QString &variant);
    void setConfiguredArchitectures(const QStringList &architectures);
    QString profile() const;

    void parseProject();
    void build();
    void finish();

    void updateState();
    void changeBuildVariant();
    QStringList configuredArchitectures() const;

    QVariantMap m_qbsConfiguration;
    Utils::SelectionAspect *m_buildVariant = nullptr;
    ArchitecturesAspect *m_selectedAbis = nullptr;
    Utils::IntegerAspect *m_maxJobCount = nullptr;
    Utils::BoolAspect *m_keepGoing = nullptr;
    Utils::BoolAspect *m_showCommandLines = nullptr;
    Utils::BoolAspect *m_install = nullptr;
    Utils::BoolAspect *m_cleanInstallDir = nullptr;
    Utils::BoolAspect *m_forceProbes = nullptr;
    Utils::StringAspect *m_commandLine = nullptr;

    // Temporary data:
    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;

    QbsSession *m_session = nullptr;

    QString m_currentTask;
    int m_maxProgress;
    bool m_lastWasSuccess;
    bool m_parsingProject = false;
    bool m_parsingAfterBuild = false;

    friend class QbsBuildStepConfigWidget;
};

class QbsBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsBuildStepFactory();
};

} // namespace Internal
} // namespace QbsProjectManager
