// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/guard.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QGridLayout;
class QLabel;
class QMenu;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class InfoLabel;
}

namespace ProjectExplorer {

class DeployConfiguration;
class RunConfiguration;
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
    void showAddRunConfigDialog();
    void cloneRunConfiguration();
    void removeRunConfiguration();
    void removeAllRunConfigurations();
    void activeRunConfigurationChanged();
    void renameRunConfiguration();
    void currentDeployConfigurationChanged(int index);
    void aboutToShowDeployMenu();
    void removeDeployConfiguration();
    void activeDeployConfigurationChanged();
    void renameDeployConfiguration();

    void updateRemoveToolButtons();

    QString uniqueDCName(const QString &name);
    QString uniqueRCName(const QString &name);
    void updateDeployConfiguration(DeployConfiguration *);
    void setConfigurationWidget(RunConfiguration *rc);

    void addRunControlWidgets();
    void addSubWidget(QWidget *subWidget, QLabel *label);
    void removeSubWidgets();

    void updateEnabledState();

    Target *m_target;
    QWidget *m_runConfigurationWidget = nullptr;
    RunConfiguration *m_runConfiguration = nullptr;
    QVBoxLayout *m_runLayout = nullptr;
    QWidget *m_deployConfigurationWidget = nullptr;
    QVBoxLayout *m_deployLayout = nullptr;
    BuildStepListWidget *m_deploySteps = nullptr;
    QMenu *m_addDeployMenu;
    Utils::Guard m_ignoreChanges;
    using RunConfigItem = QPair<QWidget *, QLabel *>;
    QList<RunConfigItem> m_subWidgets;

    QGridLayout *m_gridLayout;
    QComboBox *m_deployConfigurationCombo;
    QWidget *m_deployWidget;
    QComboBox *m_runConfigurationCombo;
    QPushButton *m_addDeployToolButton;
    QPushButton *m_removeDeployToolButton;
    QPushButton *m_addRunToolButton;
    QPushButton *m_removeRunToolButton;
    QPushButton *m_removeAllRunConfigsButton;
    QPushButton *m_renameRunButton;
    QPushButton *m_cloneRunButton;
    QPushButton *m_renameDeployButton;
    Utils::InfoLabel *m_disabledText;
};

} // namespace Internal
} // namespace ProjectExplorer
