// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDateTime>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
class BuildConfiguration;
class DeployConfiguration;
class RunConfiguration;

namespace Internal {
class GenericListWidget;
class ProjectListView;
class KitAreaWidget;

class MiniProjectTargetSelector : public QWidget
{
    Q_OBJECT

public:
    explicit MiniProjectTargetSelector(QAction *projectAction, QWidget *parent);

    void setVisible(bool visible) override;

    void keyPressEvent(QKeyEvent *ke) override;
    void keyReleaseEvent(QKeyEvent *ke) override;
    bool event(QEvent *event) override;

    void toggleVisible();
    void nextOrShow();

private:
    friend class ProjectExplorer::Target;
    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
    void handleNewTarget(Target *target);
    void handleRemovalOfTarget(Target *pc);

    void changeStartupProject(ProjectExplorer::Project *project);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void kitChanged(ProjectExplorer::Kit *k);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void delayedHide();
    void updateActionAndSummary();
    void switchToProjectsMode();
    void addedTarget(Target *target);
    void removedTarget(Target *target);
    void addedBuildConfiguration(BuildConfiguration *bc, bool update = true);
    void removedBuildConfiguration(BuildConfiguration *bc, bool update = true);
    void addedDeployConfiguration(DeployConfiguration *dc, bool update = true);
    void removedDeployConfiguration(DeployConfiguration *dc, bool update = true);
    void addedRunConfiguration(RunConfiguration *rc, bool update = true);
    void removedRunConfiguration(RunConfiguration *rc, bool update = true);

    void updateProjectListVisible();
    void updateTargetListVisible();
    void updateBuildListVisible();
    void updateDeployListVisible();
    void updateRunListVisible();
    void updateSummary();
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;

    void doLayout(bool keepSize);
    QVector<int> listWidgetWidths(int minSize, int maxSize);
    QWidget *createTitleLabel(const QString &text);

    QAction *m_projectAction;

    enum TYPES { PROJECT = 0, TARGET = 1, BUILD = 2, DEPLOY = 3, RUN = 4, LAST = 5 };
    ProjectListView *m_projectListWidget;
    KitAreaWidget *m_kitAreaWidget;
    QVector<GenericListWidget *> m_listWidgets;
    QVector<QWidget *> m_titleWidgets;
    QLabel *m_summaryLabel;

    Project *m_project = nullptr;
    Target *m_target = nullptr;
    BuildConfiguration *m_buildConfiguration = nullptr;
    DeployConfiguration *m_deployConfiguration = nullptr;
    RunConfiguration *m_runConfiguration = nullptr;
    bool m_hideOnRelease = false;
    QDateTime m_earliestHidetime;
};

} // namespace Internal
} // namespace ProjectExplorer
