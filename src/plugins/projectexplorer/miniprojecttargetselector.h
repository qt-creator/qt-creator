/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    explicit ProjectListWidget(ProjectExplorer::Project *project, QWidget *parent = 0);

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
    explicit MiniTargetWidget(ProjectExplorer::Target *target, QWidget *parent = 0);
    ProjectExplorer::Target *target() const;

    bool hasBuildConfiguration() const;

private slots:
    void addRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);
    void removeRunConfiguration(ProjectExplorer::RunConfiguration *runConfig);
    void addBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);
    void removeBuildConfiguration(ProjectExplorer::BuildConfiguration *buildConfig);

    void setActiveBuildConfiguration(int index);
    void setActiveRunConfiguration(int index);
    void setActiveBuildConfiguration();
    void setActiveRunConfiguration();

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
    explicit MiniProjectTargetSelector(QAction *projectAction, QWidget *parent = 0);
    void setVisible(bool visible);

signals:
    void startupProjectChanged(ProjectExplorer::Project *project);

private slots:
    void addProject(ProjectExplorer::Project *project);
    void removeProject(ProjectExplorer::Project *project);
    void addTarget(ProjectExplorer::Target *target, bool isActiveTarget = false);
    void removeTarget(ProjectExplorer::Target *target);
    void changeActiveTarget(ProjectExplorer::Target *target);
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
    bool m_ignoreIndexChange;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // MINIPROJECTTARGETSELECTOR_H
