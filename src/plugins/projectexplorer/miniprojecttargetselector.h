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

#include <QListWidget>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QComboBox;
class QGridLayout;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class KitConfigWidget;
class Project;
class Target;
class BuildConfiguration;
class DeployConfiguration;
class ProjectConfiguration;
class RunConfiguration;

namespace Internal {

// helper classes
class ListWidget : public QListWidget
{
    Q_OBJECT

public:
    ListWidget(QWidget *parent);
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void setMaxCount(int maxCount);
    int maxCount();

    int optimalWidth() const;
    void setOptimalWidth(int width);

    int padding();

private:
    int m_maxCount = 0;
    int m_optimalWidth = 0;
};

class ProjectListWidget : public ListWidget
{
    Q_OBJECT

public:
    explicit ProjectListWidget(QWidget *parent = nullptr);

private:
    void addProject(ProjectExplorer::Project *project);
    void removeProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void changeStartupProject(ProjectExplorer::Project *project);
    void setProject(int index);
    QListWidgetItem *itemForProject(Project *project);
    QString fullName(Project *project);
    bool m_ignoreIndexChange;
};

class KitAreaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit KitAreaWidget(QWidget *parent = nullptr);
    ~KitAreaWidget() override;

    void setKit(ProjectExplorer::Kit *k);

private:
    void updateKit(ProjectExplorer::Kit *k);

    QGridLayout *m_layout;
    Kit *m_kit = nullptr;
    QList<KitConfigWidget *> m_widgets;
    QList<QLabel *> m_labels;
};

class GenericListWidget : public ListWidget
{
    Q_OBJECT

public:
    explicit GenericListWidget(QWidget *parent = nullptr);

signals:
    void changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration *dc);

public:
    void setProjectConfigurations(const QList<ProjectConfiguration *> &list, ProjectConfiguration *active);
    void setActiveProjectConfiguration(ProjectConfiguration *active);
    void addProjectConfiguration(ProjectConfiguration *pc);
    void removeProjectConfiguration(ProjectConfiguration *pc);

private:
    void rowChanged(int index);
    void displayNameChanged();
    QListWidgetItem *itemForProjectConfiguration(ProjectConfiguration *pc);
    bool m_ignoreIndexChange;
};

// main class
class MiniProjectTargetSelector : public QWidget
{
    Q_OBJECT

public:
    explicit MiniProjectTargetSelector(QAction *projectAction, QWidget *parent = nullptr);
    void setVisible(bool visible) override;

    void keyPressEvent(QKeyEvent *ke) override;
    void keyReleaseEvent(QKeyEvent *ke) override;
    bool event(QEvent *event) override;

    void toggleVisible();
    void nextOrShow();

private:
    void projectAdded(ProjectExplorer::Project *project);
    void projectRemoved(ProjectExplorer::Project *project);
    void slotAddedTarget(ProjectExplorer::Target *target);
    void slotRemovedTarget(ProjectExplorer::Target *target);
    void slotAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void slotRemovedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void slotAddedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void slotRemovedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void slotAddedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void slotRemovedRunConfiguration(ProjectExplorer::RunConfiguration *rc);

    void changeStartupProject(ProjectExplorer::Project *project);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void kitChanged(ProjectExplorer::Kit *k);
    void activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration *bc);
    void activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration *dc);
    void activeRunConfigurationChanged(ProjectExplorer::RunConfiguration *rc);

    void setActiveTarget(ProjectExplorer::ProjectConfiguration *pc);
    void setActiveBuildConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void setActiveDeployConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void setActiveRunConfiguration(ProjectExplorer::ProjectConfiguration *pc);

    void delayedHide();
    void updateActionAndSummary();
    void switchToProjectsMode();
    void addedTarget(Target *target);
    void removedTarget(Target *target);
    void addedBuildConfiguration(BuildConfiguration* bc);
    void removedBuildConfiguration(BuildConfiguration* bc);
    void addedDeployConfiguration(DeployConfiguration *dc);
    void removedDeployConfiguration(DeployConfiguration *dc);
    void addedRunConfiguration(RunConfiguration *rc);
    void removedRunConfiguration(RunConfiguration *rc);

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
    ProjectListWidget *m_projectListWidget;
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
