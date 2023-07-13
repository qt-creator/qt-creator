// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qbsbuildconfiguration.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/task.h>

namespace QbsProjectManager {
namespace Internal {

class ErrorInfo;
class QbsProject;
class QbsSession;

class QbsBuildStepConfigWidget;

class ArchitecturesAspect : public Utils::MultiSelectionAspect
{
    Q_OBJECT

public:
    ArchitecturesAspect(Utils::AspectContainer *container = nullptr);

    void setKit(const ProjectExplorer::Kit *kit) { m_kit = kit; }
    void addToLayout(Layouting::LayoutItem &parent) override;
    QStringList selectedArchitectures() const;
    void setSelectedArchitectures(const QStringList& architectures);
    bool isManagedByTarget() const { return m_isManagedByTarget; }

private:
    void setVisibleDynamic(bool visible);

    const ProjectExplorer::Kit *m_kit = nullptr;
    QMap<QString, QString> m_abisToArchMap;
    bool m_isManagedByTarget = false;
};

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

    bool hasCustomInstallRoot() const;
    Utils::FilePath installRoot(VariableHandling variableHandling = ExpandVariables) const;
    QString buildVariant() const;
    int maxJobs() const;

    Utils::SelectionAspect buildVariantHolder{this};
    ArchitecturesAspect selectedAbis{this};
    Utils::IntegerAspect maxJobCount{this};
    Utils::BoolAspect keepGoing{this};
    Utils::BoolAspect showCommandLines{this};
    Utils::BoolAspect install{this};
    Utils::BoolAspect cleanInstallRoot{this};
    Utils::BoolAspect forceProbes{this};
    Utils::StringAspect commandLine{this};

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
    QStringList configuredArchitectures() const;

    QVariantMap m_qbsConfiguration;

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
