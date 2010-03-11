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

#ifndef PROJECTWINDOW_H
#define PROJECTWINDOW_H

#include "iprojectproperties.h"

#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtGui/QApplication>
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
class Target;
class BuildConfiguration;
class RunConfiguration;

namespace Internal {

class DoubleTabWidget;

class PanelsWidget : public QScrollArea
{
    Q_OBJECT
public:
    PanelsWidget(QWidget *parent);
    ~PanelsWidget();
    // Adds a widget
    void addPropertiesPanel(IPropertiesPanel *panel);

private:
    void addPanelWidget(IPropertiesPanel *panel, int row);

    QList<IPropertiesPanel *> m_panels;
    QGridLayout *m_layout;
    QWidget *m_root;
};

class ProjectWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ProjectWindow(QWidget *parent = 0);
    ~ProjectWindow();

    void shutdown();
private slots:
    void showProperties(int index, int subIndex);
    void restoreStatus();
    void saveStatus();
    void registerProject(ProjectExplorer::Project*);
    void deregisterProject(ProjectExplorer::Project*);

    void refreshProject();

private:
    void removeCurrentWidget();

    DoubleTabWidget *m_tabWidget;
    QStackedWidget *m_centralWidget;
    QWidget *m_currentWidget;
    IPropertiesPanel *m_currentPanel;
    QList<ProjectExplorer::Project *> m_tabIndexToProject;
};


} // namespace Internal
} // namespace ProjectExplorer

#endif // PROJECTWINDOW_H
