// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#include "cmakeabstractprocessstep.h"
#include <utils/treemodel.h>

namespace Utils {
class CommandLine;
class StringAspect;
} // Utils

namespace CMakeProjectManager::Internal {

class CMakeBuildStep;

class CMakeTargetItem : public Utils::TreeItem
{
public:
    CMakeTargetItem() = default;
    CMakeTargetItem(const QString &target, CMakeBuildStep *step, bool special);

private:
    QVariant data(int column, int role) const final;
    bool setData(int column, const QVariant &data, int role) final;
    Qt::ItemFlags flags(int column) const final;

    QString m_target;
    CMakeBuildStep *m_step = nullptr;
    bool m_special = false;
};

class CMakeBuildStep : public CMakeAbstractProcessStep
{
    Q_OBJECT

public:
    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    QStringList buildTargets() const;
    void setBuildTargets(const QStringList &target) override;

    bool buildsBuildTarget(const QString &target) const;
    void setBuildsBuildTarget(const QString &target, bool on);

    void toMap(Utils::Store &map) const override;

    QString cleanTarget() const;
    QString allTarget() const ;
    QString installTarget() const;
    static QStringList specialTargets(bool allCapsTargets);

    QString activeRunConfigTarget() const;

    void setBuildPreset(const QString &preset);

    Utils::Environment environment() const;
    void setUserEnvironmentChanges(const Utils::EnvironmentItems &diff);
    Utils::EnvironmentItems userEnvironmentChanges() const;
    bool useClearEnvironment() const;
    void setUseClearEnvironment(bool b);
    void updateAndEmitEnvironmentChanged();

    Utils::Environment baseEnvironment() const;
    QString baseEnvironmentText() const;

    void setCMakeArguments(const QStringList &cmakeArguments);
    void setToolArguments(const QStringList &nativeToolArguments);

    void setConfiguration(const QString &configuration);

    Utils::StringAspect cmakeArguments{this};
    Utils::StringAspect toolArguments{this};
    Utils::BoolAspect useiOSAutomaticProvisioningUpdates{this};
    Utils::BoolAspect useStaging{this};
    Utils::FilePathAspect stagingDir{this};

signals:
    void buildTargetsChanged();
    void environmentChanged();

private:
    Utils::CommandLine cmakeCommand() const;

    void fromMap(const Utils::Store &map) override;

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    Tasking::GroupItem runRecipe() final;
    QWidget *createConfigWidget() override;

    Utils::FilePath cmakeExecutable() const;
    QString currentInstallPrefix() const;

    QString defaultBuildTarget() const;
    bool isCleanStep() const;

    void handleBuildTargetsChanges(bool success);
    void recreateBuildTargetsModel();
    void updateBuildTargetsModel();
    void updateDeploymentData();

    friend class CMakeBuildStepConfigWidget;
    QStringList m_buildTargets; // Convention: Empty string member signifies "Current executable"

    QString m_allTarget = "all";
    QString m_installTarget = "install";

    Utils::TreeModel<Utils::TreeItem, CMakeTargetItem> m_buildTargetModel;

    Utils::Environment m_environment;
    Utils::EnvironmentItems  m_userEnvironmentChanges;
    bool m_clearSystemEnvironment = false;
    QString m_buildPreset;
    std::optional<QString> m_configuration;
};

void setupCMakeBuildStep();

} // CMakeProjectManager::Internal
