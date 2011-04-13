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

#ifndef RUNSETTINGSPROPERTIESPAGE_H
#define RUNSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QtGui/QWidget>
#include <QtGui/QIcon>

QT_BEGIN_NAMESPACE
class QMenu;
class QLabel;
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer {

class DeployConfiguration;
class DeployConfigurationWidget;
class DeployConfigurationModel;
class RunConfiguration;
class RunConfigurationModel;
class RunConfigWidget;

namespace Internal {

const char * const RUNSETTINGS_PANEL_ID = "ProjectExplorer.RunSettingsPanel";

namespace Ui {
class RunSettingsPropertiesPage;
}

class RunSettingsWidget;
class BuildStepListWidget;

class RunSettingsPanelFactory : public ITargetPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    bool supports(Target *target);
    IPropertiesPanel *createPanel(Target *target);
};

class RunSettingsPanel : public IPropertiesPanel
{
public:
    RunSettingsPanel(Target *target);
    ~RunSettingsPanel();

    QString displayName() const;
    QWidget *widget() const;
    QIcon icon() const;

private:
    RunSettingsWidget *m_widget;
    QIcon m_icon;
};

class RunSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RunSettingsWidget(Target *target);
    ~RunSettingsWidget();

private slots:
    void currentRunConfigurationChanged(int index);
    void aboutToShowAddMenu();
    void addRunConfiguration();
    void removeRunConfiguration();
    void activeRunConfigurationChanged();
    void renameRunConfiguration();
    void currentDeployConfigurationChanged(int index);
    void aboutToShowDeployMenu();
    void addDeployConfiguration();
    void removeDeployConfiguration();
    void activeDeployConfigurationChanged();
    void renameDeployConfiguration();

private:
    QString uniqueDCName(const QString &name);
    QString uniqueRCName(const QString &name);
    void updateDeployConfiguration(DeployConfiguration *);
    void setConfigurationWidget(RunConfiguration *rc);

    void addRunControlWidgets();
    void addSubWidget(RunConfigWidget *subWidget);
    void removeSubWidgets();

    Target *m_target;
    RunConfigurationModel *m_runConfigurationsModel;
    DeployConfigurationModel *m_deployConfigurationModel;
    Ui::RunSettingsPropertiesPage *m_ui;
    QWidget *m_runConfigurationWidget;
    QVBoxLayout *m_runLayout;
    DeployConfigurationWidget *m_deployConfigurationWidget;
    QVBoxLayout *m_deployLayout;
    BuildStepListWidget *m_deploySteps;
    QMenu *m_addRunMenu;
    QMenu *m_addDeployMenu;
    bool m_ignoreChange;
    typedef QPair<RunConfigWidget *, QLabel *> RunConfigItem;
    QList<RunConfigItem> m_subWidgets;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // RUNSETTINGSPROPERTIESPAGE_H
