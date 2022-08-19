// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/abstractprocessstep.h>
#include <utils/treemodel.h>

namespace Utils {
class CommandLine;
class StringAspect;
} // Utils

namespace CMakeProjectManager {
namespace Internal {

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

class CMakeBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT

public:
    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    QStringList buildTargets() const;
    void setBuildTargets(const QStringList &target);

    bool buildsBuildTarget(const QString &target) const;
    void setBuildsBuildTarget(const QString &target, bool on);

    QVariantMap toMap() const override;

    QString cleanTarget() const;
    QString allTarget() const ;
    QString installTarget() const;
    static QStringList specialTargets(bool allCapsTargets);

    QString activeRunConfigTarget() const;

signals:
    void buildTargetsChanged();

private:
    Utils::CommandLine cmakeCommand() const;

    void processFinished(int exitCode, QProcess::ExitStatus status) override;
    bool fromMap(const QVariantMap &map) override;

    bool init() override;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) override;
    void doRun() override;
    QWidget *createConfigWidget() override;

    QString defaultBuildTarget() const;

    void runImpl();
    void handleProjectWasParsed(bool success);

    void handleBuildTargetsChanges(bool success);
    void recreateBuildTargetsModel();
    void updateBuildTargetsModel();

    QMetaObject::Connection m_runTrigger;

    friend class CMakeBuildStepConfigWidget;
    QStringList m_buildTargets; // Convention: Empty string member signifies "Current executable"
    Utils::StringAspect *m_cmakeArguments = nullptr;
    Utils::StringAspect *m_toolArguments = nullptr;
    Utils::BoolAspect *m_useiOSAutomaticProvisioningUpdates = nullptr;
    bool m_waiting = false;

    QString m_allTarget = "all";
    QString m_installTarget = "install";

    Utils::TreeModel<Utils::TreeItem, CMakeTargetItem> m_buildTargetModel;
};

class CMakeBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeBuildStepFactory();
};

} // namespace Internal
} // namespace CMakeProjectManager
