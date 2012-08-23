/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef RUNSETTINGSPROPERTIESPAGE_H
#define RUNSETTINGSPROPERTIESPAGE_H

#include "iprojectproperties.h"

#include <QWidget>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QComboBox;
class QGridLayout;
class QLabel;
class QMenu;
class QPushButton;
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

const char RUNSETTINGS_PANEL_ID[] = "ProjectExplorer.RunSettingsPanel";

class BuildStepListWidget;

class RunSettingsPanelFactory : public ITargetPanelFactory
{
public:
    QString id() const;
    QString displayName() const;
    int priority() const;
    bool supports(Target *target);
    PropertiesPanel *createPanel(Target *target);
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

    QGridLayout *m_gridLayout;
    QComboBox *m_deployConfigurationCombo;
    QWidget *m_deployWidget;
    QComboBox *m_runConfigurationCombo;
    QPushButton *m_addDeployToolButton;
    QPushButton *m_removeDeployToolButton;
    QPushButton *m_addRunToolButton;
    QPushButton *m_removeRunToolButton;
    QPushButton *m_renameRunButton;
    QPushButton *m_renameDeployButton;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // RUNSETTINGSPROPERTIESPAGE_H
