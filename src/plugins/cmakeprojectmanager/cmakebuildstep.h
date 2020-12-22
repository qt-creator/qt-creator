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

    static QString cleanTarget();
    static QString allTarget();
    static QString installTarget();
    static QString installStripTarget();
    static QString packageTarget();
    static QString testTarget();
    static QStringList specialTargets();

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
    bool m_waiting = false;

    Utils::TreeModel<Utils::TreeItem, CMakeTargetItem> m_buildTargetModel;
};

class CMakeBuildStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    CMakeBuildStepFactory();
};

} // namespace Internal
} // namespace CMakeProjectManager
