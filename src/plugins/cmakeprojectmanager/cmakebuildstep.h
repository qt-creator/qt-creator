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

#include <QRegExp>

QT_BEGIN_NAMESPACE
class QLineEdit;
class QListWidget;
class QListWidgetItem;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace ProjectExplorer { class ToolChain; }

namespace CMakeProjectManager {
namespace Internal {

class CMakeBuildConfiguration;
class CMakeRunConfiguration;
class CMakeBuildStepFactory;

class CMakeBuildStep : public ProjectExplorer::AbstractProcessStep
{
    Q_OBJECT
    friend class CMakeBuildStepFactory;

public:
    explicit CMakeBuildStep(ProjectExplorer::BuildStepList *bsl);

    CMakeBuildConfiguration *cmakeBuildConfiguration() const;
    CMakeBuildConfiguration *targetsActiveBuildConfiguration() const;

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;

    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;

    QString buildTarget() const;
    bool buildsBuildTarget(const QString &target) const;
    void setBuildTarget(const QString &target);
    void clearBuildTargets();

    QString toolArguments() const;
    void setToolArguments(const QString &list);

    QString allArguments(const CMakeRunConfiguration *rc) const;

    QString cmakeCommand() const;

    QVariantMap toMap() const override;

    static QString cleanTarget();
    static QString allTarget();

signals:
    void cmakeCommandChanged();
    void targetToBuildChanged();
    void buildTargetsChanged();

protected:
    void processStarted() override;
    void processFinished(int exitCode, QProcess::ExitStatus status) override;

    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, CMakeBuildStep *bs);
    CMakeBuildStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool fromMap(const QVariantMap &map) override;

    // For parsing [ 76%]
    void stdOutput(const QString &line) override;

private:
    void ctor(ProjectExplorer::BuildStepList *bsl);

    void runImpl(QFutureInterface<bool> &fi);

    void handleBuildTargetChanges();
    CMakeRunConfiguration *targetsActiveRunConfiguration() const;

    QMetaObject::Connection m_runTrigger;
    QMetaObject::Connection m_errorTrigger;

    QRegExp m_percentProgress;
    QRegExp m_ninjaProgress;
    QString m_ninjaProgressString;
    QString m_buildTarget;
    QString m_toolArguments;
    bool m_useNinja = false;
};

class CMakeBuildStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    CMakeBuildStepConfigWidget(CMakeBuildStep *buildStep);
    QString displayName() const override;
    QString summaryText() const override;

private:
    void itemChanged(QListWidgetItem*);
    void toolArgumentsEdited();
    void updateDetails();
    void buildTargetsChanged();
    void selectedBuildTargetsChanged();

    CMakeBuildStep *m_buildStep;
    QLineEdit *m_toolArguments;
    QListWidget *m_buildTargetsList;
    QString m_summaryText;
};

class CMakeBuildStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT

public:
    explicit CMakeBuildStepFactory(QObject *parent = 0);

    QList<ProjectExplorer::BuildStepInfo>
        availableSteps(ProjectExplorer::BuildStepList *parent) const override;

    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, Core::Id id) override;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) override;
};

} // namespace Internal
} // namespace CMakeProjectManager
