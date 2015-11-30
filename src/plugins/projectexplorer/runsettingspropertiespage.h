/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef RUNSETTINGSPROPERTIESPAGE_H
#define RUNSETTINGSPROPERTIESPAGE_H

#include <QWidget>

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
class DeployConfigurationModel;
class NamedWidget;
class RunConfiguration;
class RunConfigurationModel;
class RunConfigWidget;
class Target;

namespace Internal {

const char RUNSETTINGS_PANEL_ID[] = "ProjectExplorer.RunSettingsPanel";

class BuildStepListWidget;

class RunSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RunSettingsWidget(Target *target);
    ~RunSettingsWidget();

private slots:
    void currentRunConfigurationChanged(int index);
    void aboutToShowAddMenu();
    void cloneRunConfiguration();
    void removeRunConfiguration();
    void activeRunConfigurationChanged();
    void renameRunConfiguration();
    void currentDeployConfigurationChanged(int index);
    void aboutToShowDeployMenu();
    void removeDeployConfiguration();
    void activeDeployConfigurationChanged();
    void renameDeployConfiguration();

    void updateRemoveToolButton();

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
    RunConfiguration *m_runConfiguration;
    QVBoxLayout *m_runLayout;
    NamedWidget *m_deployConfigurationWidget;
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
