// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace QbsProjectManager::Internal {

class QbsBuildConfiguration;
class QbsBuildStepConfigWidget;
class QbsBuildStepData;
class QbsBuildSystem;

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

    QVariantMap qbsConfiguration(VariableHandling variableHandling) const;
    void setQbsConfiguration(const QVariantMap &config);

    Utils::FilePath installRoot(VariableHandling variableHandling = ExpandVariables) const;
    QString buildVariant() const;

    Utils::SelectionAspect buildVariantHolder{this};
    ArchitecturesAspect selectedAbis{this};
    Utils::IntegerAspect maxJobCount{this};
    Utils::BoolAspect keepGoing{this};
    Utils::BoolAspect showCommandLines{this};
    Utils::BoolAspect install{this};
    Utils::BoolAspect cleanInstallRoot{this};
    Utils::BoolAspect forceProbes{this};
    Utils::StringAspect commandLine{this};

signals:
    void qbsConfigurationChanged();

private:
    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    Tasking::GroupItem runRecipe() final;
    QWidget *createConfigWidget() override;
    void fromMap(const Utils::Store &map) override;
    void toMap(Utils::Store &map) const override;

    QbsBuildConfiguration *qbsBuildConfiguration() const;
    QbsBuildSystem *qbsBuildSystem() const;
    QbsBuildStepData stepData() const;
    bool hasCustomInstallRoot() const;
    int maxJobs() const;

    void updateState();
    QStringList configuredArchitectures() const;

    QVariantMap m_qbsConfiguration;

    // Temporary data:
    QStringList m_changedFiles;
    QStringList m_activeFileTags;
    QStringList m_products;

    friend class QbsBuildStepConfigWidget;
};

class QbsBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    QbsBuildStepFactory();
};

} // namespace QbsProjectManager::Internal
