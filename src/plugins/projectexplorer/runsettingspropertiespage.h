/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef RUNSETTINGSPROPERTIESPAGE_H
#define RUNSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QtGui/QWidget>

namespace ProjectExplorer {
namespace Internal {

namespace Ui {
class RunSettingsPropertiesPage;
}

class RunConfigurationsModel;
class RunSettingsWidget;

class RunSettingsPanelFactory : public IPanelFactory
{
public:
    virtual bool supports(Project *project);
    PropertiesPanel *createPanel(Project *project);
};

class RunSettingsPanel : public PropertiesPanel
{
    Q_OBJECT
public:
    RunSettingsPanel(Project *project);
    ~RunSettingsPanel();

    QString name() const;
    QWidget *widget();
private:
    RunSettingsWidget *m_widget;
};

class RunSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    RunSettingsWidget(Project *project);
    ~RunSettingsWidget();

private slots:
    void activateRunConfiguration(int index);
    void aboutToShowAddMenu();
    void addRunConfiguration();
    void removeRunConfiguration();
    void nameChanged();

private:
    void initRunConfigurationComboBox();
    Project *m_project;
    RunConfigurationsModel *m_runConfigurationsModel;
    Ui::RunSettingsPropertiesPage *m_ui;
    QWidget *m_runConfigurationWidget;
    QMenu *m_addMenu;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // RUNSETTINGSPROPERTIESPAGE_H
