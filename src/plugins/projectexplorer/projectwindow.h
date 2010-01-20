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

#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QStackedWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QGridLayout;
class QMenu;
QT_END_NAMESPACE

namespace ProjectExplorer {

class IPropertiesPanel;
class Project;
class BuildConfiguration;
class RunConfiguration;

namespace Internal {

class PanelsWidget : public QScrollArea
{
    Q_OBJECT
public:
    PanelsWidget(QWidget *parent);
    ~PanelsWidget();
    // Adds a widget
    void addWidget(QWidget *widget);
    void addWidget(const QString &name, QWidget *widget, const QIcon &icon);

    QWidget *rootWidget() const;

    // Removes all widgets and deletes them
    void clear();

private:
    struct Panel
    {
        // This does not take ownership of widget!
        explicit Panel(QWidget *widget);
        ~Panel();

        QLabel *iconLabel;
        QWidget *lineWidget;
        QLabel *nameLabel;
        QWidget *panelWidget;
    };
    QList<Panel *> m_panels;

    void addPanelWidget(Panel *panel, int row);

    QGridLayout *m_layout;
    QWidget *m_root;
};

class BuildConfigurationComboBox : public QStackedWidget
{
    Q_OBJECT
public:
    BuildConfigurationComboBox(ProjectExplorer::Project *p, QWidget *parent = 0);
    ~BuildConfigurationComboBox();
private slots:
    void displayNameChanged();
    void activeConfigurationChanged();
    void addedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void removedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc);
    void changedIndex(int newIndex);
private:
    int buildConfigurationToIndex(BuildConfiguration *bc);
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
    void addedRunConfiguration(ProjectExplorer::RunConfiguration *);
    void removedRunConfiguration(ProjectExplorer::RunConfiguration *);
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
    void projectAdded();
    void projectRemoved();

private:
    void updateRunConfigurationsComboBox();

    ActiveConfigurationWidget *m_activeConfigurationWidget;
    QWidget *m_spacerBetween;
    QWidget *m_projectChooser;
    QLabel *m_noprojectLabel;
    PanelsWidget *m_panelsWidget;
    QList<IPropertiesPanel *> m_panels;
};


} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
