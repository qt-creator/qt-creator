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

namespace Utils { class FancyLineEdit; }

namespace QbsProjectManager {
namespace Internal {
class ErrorInfo;
class QbsProject;
class QbsSession;

class QbsBuildStepConfigWidget;

class QbsBuildStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    enum VariableHandling
    {
        PreserveVariables,
        ExpandVariables
    };

    QbsBuildStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    ~QbsBuildStep() override;

    QVariantMap qbsConfiguration(VariableHandling variableHandling) const;
    void setQbsConfiguration(const QVariantMap &config);

    bool keepGoing() const { return m_keepGoing; }
    bool showCommandLines() const { return m_showCommandLines; }
    bool install() const { return m_install; }
    bool cleanInstallRoot() const { return m_cleanInstallDir; }
    bool hasCustomInstallRoot() const;
    Utils::FilePath installRoot(VariableHandling variableHandling = ExpandVariables) const;
    int maxJobs() const;
    QString buildVariant() const;

    void setForceProbes(bool force) { m_forceProbes = force; emit qbsConfigurationChanged(); }
    bool forceProbes() const { return m_forceProbes; }

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
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
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
    QString profile() const;

    void setKeepGoing(bool kg);
    void setMaxJobs(int jobcount);
    void setShowCommandLines(bool show);
    void setInstall(bool install);
    void setCleanInstallRoot(bool clean);

    void parseProject();
    void build();
    void finish();

    QVariantMap m_qbsConfiguration;
    int m_maxJobCount = 0;
    bool m_keepGoing = false;
    bool m_showCommandLines = false;
    bool m_install = true;
    bool m_cleanInstallDir = false;
    bool m_forceProbes = false;

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
