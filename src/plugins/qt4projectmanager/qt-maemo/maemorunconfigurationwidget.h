/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef MAEMORUNCONFIGURATIONWIDGET_H
#define MAEMORUNCONFIGURATIONWIDGET_H

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QModelIndex;
class QPushButton;
class QRadioButton;
class QTableView;
class QToolButton;
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils {
class EnvironmentItem;
}

namespace ProjectExplorer {
class EnvironmentWidget;
}

namespace Utils { class DetailsWidget; }

namespace Qt4ProjectManager {
class Qt4BuildConfiguration;

namespace Internal {
class MaemoDeviceEnvReader;
class MaemoRunConfiguration;

class MaemoRunConfigurationWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MaemoRunConfigurationWidget(MaemoRunConfiguration *runConfiguration,
                                         QWidget *parent = 0);

private slots:
    void runConfigurationEnabledChange(bool enabled);
    void argumentsEdited(const QString &args);
    void showDeviceConfigurationsDialog(const QString &link);
    void updateTargetInformation();
    void handleCurrentDeviceConfigChanged();
    void addMount();
    void removeMount();
    void changeLocalMountDir(const QModelIndex &index);
    void enableOrDisableRemoveMountSpecButton();
    void handleDebuggingTypeChanged(bool useGdb);
    void fetchEnvironment();
    void fetchEnvironmentFinished();
    void fetchEnvironmentError(const QString &error);
    void stopFetchEnvironment();
    void userChangesEdited();
    void baseEnvironmentSelected(int index);
    void baseEnvironmentChanged();
    void systemEnvironmentChanged();
    void userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges);
    void handleRemoteMountsChanged();
    void handleDebuggingTypeChanged();
    void handleDeploySpecsChanged();
    void handleBuildConfigChanged();
    void handleToolchainChanged();
    void handleActiveDeployConfigurationChanged();

private:
    void addGenericWidgets(QVBoxLayout *mainLayout);
    void addDebuggingWidgets(QVBoxLayout *mainLayout);
    void addMountWidgets(QVBoxLayout *mainLayout);
    void addEnvironmentWidgets(QVBoxLayout *mainLayout);
    void updateMountWarning();

    QLineEdit *m_argsLineEdit;
    QLabel *m_localExecutableLabel;
    QLabel *m_remoteExecutableLabel;
    QLabel *m_devConfLabel;
    QLabel *m_debuggingLanguagesLabel;
    QRadioButton *m_debugCppOnlyButton;
    QRadioButton *m_debugQmlOnlyButton;
    QRadioButton *m_debugCppAndQmlButton;
    QLabel *m_mountWarningLabel;
    QTableView *m_mountView;
    QToolButton *m_removeMountButton;
    Utils::DetailsWidget *m_mountDetailsContainer;
    Utils::DetailsWidget *m_debugDetailsContainer;
    MaemoRunConfiguration *m_runConfiguration;

    bool m_ignoreChange;
    QPushButton *m_fetchEnv;
    QComboBox *m_baseEnvironmentComboBox;
    MaemoDeviceEnvReader *m_deviceEnvReader;
    ProjectExplorer::EnvironmentWidget *m_environmentWidget;
    Qt4BuildConfiguration *m_lastActiveBuildConfig;
    bool m_deployablesConnected;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // MAEMORUNCONFIGURATIONWIDGET_H
