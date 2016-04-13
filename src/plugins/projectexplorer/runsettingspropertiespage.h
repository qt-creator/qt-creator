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

class BuildStepListWidget;

class RunSettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RunSettingsWidget(Target *target);

private:
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
    QWidget *m_runConfigurationWidget = nullptr;
    RunConfiguration *m_runConfiguration = nullptr;
    QVBoxLayout *m_runLayout = nullptr;
    NamedWidget *m_deployConfigurationWidget = nullptr;
    QVBoxLayout *m_deployLayout = nullptr;
    BuildStepListWidget *m_deploySteps = nullptr;
    QMenu *m_addRunMenu;
    QMenu *m_addDeployMenu;
    bool m_ignoreChange = false;
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
