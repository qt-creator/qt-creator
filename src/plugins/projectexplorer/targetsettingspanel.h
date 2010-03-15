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

#ifndef TARGETSETTINGSPANEL_H
#define TARGETSETTINGSPANEL_H

#include "iprojectproperties.h"

#include <QtGui/QStackedWidget>
#include <QtGui/QWidget>

namespace ProjectExplorer {

class Target;

namespace Internal {

const char * const TARGETSETTINGS_PANEL_ID("ProjectExplorer.TargetSettingsPanel");

class TargetSettingsWidget;
class PanelsWidget;

class TargetSettingsPanelFactory : public IPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    bool supports(Project *project);
    bool supports(Target *target);
    IPropertiesPanel *createPanel(Project *project);
    IPropertiesPanel *createPanel(Target *target);
};

class TargetSettingsPanelWidget;

class TargetSettingsPanel : public IPropertiesPanel
{
public:
    TargetSettingsPanel(Project *project);
    ~TargetSettingsPanel();
    QString displayName() const;
    QWidget *widget() const;
    QIcon icon() const;
    PanelFlags flags() const { return IPropertiesPanel::NoAutomaticStyle; }

private:
    TargetSettingsPanelWidget *m_widget;
};

class TargetSettingsPanelWidget : public QWidget
{
    Q_OBJECT
public:
    TargetSettingsPanelWidget(Project *project);
    ~TargetSettingsPanelWidget();

    void setupUi();

private slots:
    void currentTargetChanged(int targetIndex, int subIndex);
    void addTarget();
    void removeTarget();
    void targetAdded(ProjectExplorer::Target *target);
    void aboutToRemoveTarget(ProjectExplorer::Target *target);
    void activeTargetChanged(ProjectExplorer::Target *target);
    void updateTargetAddAndRemoveButtons();

private:
    Target *m_currentTarget;
    Project *m_project;
    TargetSettingsWidget *m_selector;
    QStackedWidget *m_centralWidget;
    QWidget *m_noTargetLabel;
    PanelsWidget *m_panelWidgets[2];
    QList<Target *> m_targets;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // TARGETSETTINGSPANEL_H
