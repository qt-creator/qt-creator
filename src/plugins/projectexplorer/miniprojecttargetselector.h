/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MINIPROJECTTARGETSELECTOR_H
#define MINIPROJECTTARGETSELECTOR_H

#include <QListWidget>
#include <QDateTime>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Kit;
class Project;
class Target;
class BuildConfiguration;
class DeployConfiguration;
class ProjectConfiguration;
class RunConfiguration;
class SessionManager;

namespace Internal {

// helper classes
class ListWidget : public QListWidget
{
    Q_OBJECT
public:
    ListWidget(QWidget *parent);
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);
    void setMaxCount(int maxCount);
    int maxCount();

    int optimalWidth() const;
    void setOptimalWidth(int width);

    int padding();
private:
    int m_maxCount;
    int m_optimalWidth;
};

class ProjectListWidget : public ListWidget
{
    Q_OBJECT
public:
    explicit ProjectListWidget(SessionManager *sessionManager, QWidget *parent = 0);
private slots:
    void addProject(ProjectExplorer::Project *project);
    void removeProject(ProjectExplorer::Project *project);
    void projectDisplayNameChanged(ProjectExplorer::Project *project);
    void changeStartupProject(ProjectExplorer::Project *project);
    void setProject(int index);
private:
    QListWidgetItem *itemForProject(Project *project);
    QString fullName(ProjectExplorer::Project *project);
    SessionManager *m_sessionManager;
    bool m_ignoreIndexChange;
};

class GenericListWidget : public ListWidget
{
    Q_OBJECT
public:
    GenericListWidget(QWidget *parent = 0);
signals:
    void changeActiveProjectConfiguration(ProjectExplorer::ProjectConfiguration *dc);
public:
    void setProjectConfigurations(const QList<ProjectConfiguration *> &list, ProjectConfiguration *active);
    void setActiveProjectConfiguration(ProjectConfiguration *active);
    void addProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
    void removeProjectConfiguration(ProjectExplorer::ProjectConfiguration *pc);
private slots:
    void rowChanged(int index);
    void displayNameChanged();
private:
    QListWidgetItem *itemForProjectConfiguration(ProjectConfiguration *pc);
    bool m_ignoreIndexChange;
};

// main class
class MiniProjectTargetSelector : public QWidget
{
    Q_OBJECT
public:
    explicit MiniProjectTargetSelector(QAction *projectAction, SessionManager *sessionManager, QWidget *parent = 0);
    void setVisible(bool visible);

    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);
    bool event(QEvent *event);
public slots:
    void toggleVisible();
    void nextOrShow();

private slots:
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
private:
    void addedTarget(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration* bc);
    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration* bc);
    void addedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void removedDeployConfiguration(ProjectExplorer::DeployConfiguration *dc);
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *rc);
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *rc);

    void updateProjectListVisible();
    void updateTargetListVisible();
    void updateBuildListVisible();
    void updateDeployListVisible();
    void updateRunListVisible();
    void updateSummary();
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);

    void doLayout(bool keepSize);
    QVector<int> listWidgetWidths(int minSize, int maxSize);
    QWidget *createTitleLabel(const QString &text);

    QAction *m_projectAction;
    SessionManager *m_sessionManager;

    enum TYPES { PROJECT = 0, TARGET = 1, BUILD = 2, DEPLOY = 3, RUN = 4, LAST = 5 };
    ProjectListWidget *m_projectListWidget;
    QVector<GenericListWidget *> m_listWidgets;
    QVector<QWidget *> m_titleWidgets;
    QLabel *m_summaryLabel;

    Project *m_project;
    Target *m_target;
    BuildConfiguration *m_buildConfiguration;
    DeployConfiguration *m_deployConfiguration;
    RunConfiguration *m_runConfiguration;
    bool m_hideOnRelease;
    QDateTime m_earliestHidetime;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MINIPROJECTTARGETSELECTOR_H
