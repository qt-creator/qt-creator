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

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QStackedWidget;
QT_END_NAMESPACE

namespace Utils { class FileName; }

namespace ProjectExplorer {

class Target;
class Project;
class ProjectImporter;
class Kit;
class PanelsWidget;

namespace Internal {

class TargetSettingsWidget;

class TargetSettingsPanelWidget : public QWidget
{
    Q_OBJECT
public:
    TargetSettingsPanelWidget(Project *project);
    ~TargetSettingsPanelWidget() override;

    void setupUi();

    int currentSubIndex() const;
    void setCurrentSubIndex(int subIndex);

protected:
    bool event(QEvent *event) override;

private:
    void currentTargetChanged(int targetIndex, int subIndex);
    void showTargetToolTip(const QPoint &globalPos, int targetIndex);
    void targetAdded(ProjectExplorer::Target *target);
    void removedTarget(ProjectExplorer::Target *target);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void updateTargetButtons();
    void renameTarget();
    void openTargetPreferences();

    void removeCurrentTarget();
    void menuShown(int targetIndex);
    void addActionTriggered(QAction *action);
    void changeActionTriggered(QAction *action);
    void duplicateActionTriggered(QAction *action);
    void importTarget(const Utils::FileName &path);
    void createAction(Kit *k, QMenu *menu);

    Target *m_currentTarget = 0;
    Project *m_project;
    ProjectImporter *m_importer;
    TargetSettingsWidget *m_selector = 0;
    QStackedWidget *m_centralWidget = 0;
    QWidget *m_noTargetLabel;
    PanelsWidget *m_panelWidgets[2];
    QList<Target *> m_targets;
    QMenu *m_targetMenu;
    QMenu *m_changeMenu = 0;
    QMenu *m_duplicateMenu = 0;
    QMenu *m_addMenu;
    QAction *m_lastAction = 0;
    QAction *m_importAction = 0;
    int m_menuTargetIndex = -1;
    static int s_targetSubIndex;
};

} // namespace Internal
} // namespace ProjectExplorer
