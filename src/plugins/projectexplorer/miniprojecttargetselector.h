/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MINIPROJECTTARGETSELECTOR_H
#define MINIPROJECTTARGETSELECTOR_H

#include <QtGui/QListWidget>


QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
class Target;
class RunConfiguration;
class Target;
class BuildConfiguration;

namespace Internal {

// helper classes

class ProjectListWidget : public QListWidget
{
    Q_OBJECT
private:
    ProjectExplorer::Project* m_project;

public:
    ProjectListWidget(ProjectExplorer::Project *project, QWidget *parent = 0);

    QSize sizeHint() const;

    void setBuildComboPopup();
    void setRunComboPopup();

    ProjectExplorer::Project *project() const;

private slots:
    void setTarget(int index);
};

class MiniTargetWidget : public QWidget
{
    Q_OBJECT
public:
    MiniTargetWidget(ProjectExplorer::Target *target, QWidget *parent = 0);
    ProjectExplorer::Target *target() const;

    bool hasBuildConfiguration() const;

private slots:
    void addRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);
    void removeRunConfiguration(ProjectExplorer::RunConfiguration *buildConfig);
    void addBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);
    void removeBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);

    void setActiveBuildConfiguration(int index);
    void setActiveRunConfiguration(int index);
    void setActiveBuildConfiguration();
    void setActiveRunConfiguration();

    void updateDisplayName();
    void updateIcon();

signals:
    void changed();

private:
    QLabel *m_targetName;
    QLabel *m_targetIcon;
    QComboBox *m_runComboBox;
    QComboBox *m_buildComboBox;
    ProjectExplorer::Target *m_target;

public:
    QComboBox *runSettingsComboBox() const { return m_runComboBox; }
    QComboBox *buildSettingsComboBox() const { return m_buildComboBox; }
};

// main class

class MiniProjectTargetSelector : public QWidget
{
    Q_OBJECT
public:
    MiniProjectTargetSelector(QAction *projectAction, QWidget *parent = 0);
    void setVisible(bool visible);

signals:
    void startupProjectChanged(ProjectExplorer::Project *project);

private slots:
    void addProject(ProjectExplorer::Project *project);
    void removeProject(ProjectExplorer::Project *project);
    void addTarget(ProjectExplorer::Target *target, bool isActiveTarget = false);
    void removeTarget(ProjectExplorer::Target *target);
    void emitStartupProjectChanged(int index);
    void changeStartupProject(ProjectExplorer::Project *project);
    void updateAction();
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);

private:
    int indexFor(ProjectExplorer::Project *project) const;

    QAction *m_projectAction;
    QComboBox *m_projectsBox;
    QStackedWidget *m_widgetStack;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MINIPROJECTTARGETSELECTOR_H
