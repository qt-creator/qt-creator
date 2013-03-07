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

#ifndef TARGETSETTINGSPANEL_H
#define TARGETSETTINGSPANEL_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QStackedWidget;
QT_END_NAMESPACE

namespace ProjectExplorer {

class Target;
class Project;
class Kit;
class PanelsWidget;

namespace Internal {

class TargetSettingsWidget;

class TargetSettingsPanelWidget : public QWidget
{
    Q_OBJECT
public:
    TargetSettingsPanelWidget(Project *project);
    ~TargetSettingsPanelWidget();

    void setupUi();

    int currentSubIndex() const;
    void setCurrentSubIndex(int subIndex);

protected:
    bool event(QEvent *event);
private slots:
    void currentTargetChanged(int targetIndex, int subIndex);
    void showTargetToolTip(const QPoint &globalPos, int targetIndex);
    void targetAdded(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void updateTargetButtons();
    void renameTarget();
    void openTargetPreferences();

    void removeTarget();
    void menuShown(int targetIndex);
    void addActionTriggered(QAction *action);
    void changeActionTriggered(QAction *action);
    void duplicateActionTriggered(QAction *action);
private:
    Target *cloneTarget(Target *sourceTarget, Kit *k);
    void removeTarget(Target *t);
    void createAction(Kit *k, QMenu *menu);

    Target *m_currentTarget;
    Project *m_project;
    TargetSettingsWidget *m_selector;
    QStackedWidget *m_centralWidget;
    QWidget *m_noTargetLabel;
    PanelsWidget *m_panelWidgets[2];
    QList<Target *> m_targets;
    QMenu *m_targetMenu;
    QMenu *m_changeMenu;
    QMenu *m_duplicateMenu;
    QMenu *m_addMenu;
    QAction *m_lastAction;
    int m_menuTargetIndex;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSPANEL_H
