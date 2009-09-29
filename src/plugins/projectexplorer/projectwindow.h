/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include <QtGui/QWidget>
#include <QtGui/QScrollArea>
#include <QtGui/QComboBox>
#include <QtCore/QPair>
#include <QtGui/QStackedWidget>
#include <QtGui/QPushButton>
#include <QtGui/QToolButton>
#include <QtGui/QLabel>

QT_BEGIN_NAMESPACE
class QLabel;
class QVBoxLayout;
class QModelIndex;
class QTabWidget;
class QHBoxLayout;
class QComboBox;
class QMenu;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Project;
class PropertiesPanel;
class ProjectExplorerPlugin;
class SessionManager;

namespace Internal {

class PanelsWidget : public QScrollArea
{
    Q_OBJECT
public:
    PanelsWidget(QWidget *parent);
    ~PanelsWidget();
    // Adds a widget
    void addWidget(QWidget *widget);
    void addWidget(const QString &name, QWidget *widget);
    void removeWidget(QWidget *widget);

    // Removes all widgets and deletes them
    void clear();
private:

    struct Panel
    {
        QLabel *nameLabel;
        QWidget *panelWidget;
        QHBoxLayout *marginLayout;
    };
    QVBoxLayout *m_layout;
    QList<Panel> m_panels;
};

class BuildConfigurationComboBox : public QStackedWidget
{
    Q_OBJECT
public:
    BuildConfigurationComboBox(ProjectExplorer::Project *p, QWidget *parent = 0);
    ~BuildConfigurationComboBox();
private slots:
    void nameChanged(const QString &buildConfiguration);
    void activeConfigurationChanged();
    void addedBuildConfiguration(ProjectExplorer::Project *, const QString &buildConfiguration);
    void removedBuildConfiguration(ProjectExplorer::Project *, const QString &buildConfiguration);
    void changedIndex(int newIndex);
private:
    int nameToIndex(const QString &buildConfiguration);
    bool ignoreIndexChange;
    ProjectExplorer::Project *m_project;
    QComboBox *m_comboBox;
    QLabel *m_label;
};

class ActiveConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    ActiveConfigurationWidget(QWidget *parent = 0);
    ~ActiveConfigurationWidget();
private slots:
    void projectAdded(ProjectExplorer::Project*);
    void projectRemoved(ProjectExplorer::Project*);
private:
    QMap<ProjectExplorer::Project *, QPair<BuildConfigurationComboBox *, QLabel *> > m_buildComboBoxMap;
};

class RunConfigurationComboBox : public QComboBox
{
    Q_OBJECT
public:
    RunConfigurationComboBox(QWidget *parent = 0);
    ~RunConfigurationComboBox();
private slots:
    void activeRunConfigurationChanged();
    void activeItemChanged(int);
    void addedRunConfiguration(ProjectExplorer::Project *p, const QString &);
    void removedRunConfiguration(ProjectExplorer::Project *p, const QString &);
    void projectAdded(ProjectExplorer::Project*);
    void projectRemoved(ProjectExplorer::Project*);
    void rebuildTree();
private:
    int convertTreeIndexToInt(int project, int runconfigurationIndex);
    QPair<int, int> convertIntToTreeIndex(int index);
    void connectToProject(ProjectExplorer::Project *p);
    void disconnectFromProject(ProjectExplorer::Project *p);

    bool m_ignoreChange;
};

class ProjectLabel : public QLabel
{
    Q_OBJECT
public:
    ProjectLabel(QWidget *parent);
    ~ProjectLabel();
public slots:
    void setProject(ProjectExplorer::Project *);
};

class ProjectPushButton : public QPushButton
{
    Q_OBJECT
public:
    ProjectPushButton(QWidget *parent);
    ~ProjectPushButton();
signals:
    void projectChanged(ProjectExplorer::Project *);

private slots:
    void projectAdded(ProjectExplorer::Project*);
    void projectRemoved(ProjectExplorer::Project*);
    void actionTriggered();
private:
    QMenu *m_menu;
};

class ProjectWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectWindow(QWidget *parent = 0);
    ~ProjectWindow();

private slots:
    void showProperties(ProjectExplorer::Project *project);
    void restoreStatus();
    void saveStatus();

private:
    void updateRunConfigurationsComboBox();
    SessionManager *m_session;
    ProjectExplorerPlugin *m_projectExplorer;

    ActiveConfigurationWidget *m_activeConfigurationWidget;
    QWidget *m_spacerBetween;
    QWidget *m_projectChooser;
    PanelsWidget *m_panelsWidget;

    Project *findProject(const QString &path) const;
    bool m_currentItemChanged;
};


} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
