/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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
