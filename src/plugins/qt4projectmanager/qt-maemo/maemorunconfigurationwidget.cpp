/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "maemorunconfigurationwidget.h"

#include "maemodeployables.h"
#include "maemodeploystep.h"
#include "maemodeviceenvreader.h"
#include "maemomanager.h"
#include "maemoglobal.h"
#include "maemoremotemountsmodel.h"
#include "maemorunconfiguration.h"
#include "maemosettingspages.h"
#include "qt4maemotarget.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/detailswidget.h>

#include <QtGui/QButtonGroup>
#include <QtCore/QCoreApplication>
#include <QtGui/QComboBox>
#include <QtGui/QFileDialog>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QTableView>
#include <QtGui/QToolButton>

namespace Qt4ProjectManager {
namespace Internal {
namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("Qt4ProjectManager::Internal::MaemoRunConfigurationWidget",
          "Fetch Device Environment");
} // anonymous namespace

MaemoRunConfigurationWidget::MaemoRunConfigurationWidget(
        MaemoRunConfiguration *runConfiguration, QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration),
    m_ignoreChange(false),
    m_deviceEnvReader(new MaemoDeviceEnvReader(this, runConfiguration)),
    m_deployablesConnected(false)
{
    m_lastActiveBuildConfig = m_runConfiguration->activeQt4BuildConfiguration();
    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    addGenericWidgets(mainLayout);
    mainLayout->addSpacing(20);
    addDebuggingWidgets(mainLayout);
    addMountWidgets(mainLayout);
    addEnvironmentWidgets(mainLayout);
    connect(m_runConfiguration,
        SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        this, SLOT(handleCurrentDeviceConfigChanged()));
    handleCurrentDeviceConfigChanged();
    connect(m_runConfiguration->maemoTarget(),
        SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
        this, SLOT(handleBuildConfigChanged()));

    connect(m_runConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));

    handleBuildConfigChanged();

    setEnabled(m_runConfiguration->isEnabled());
}

void MaemoRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    setEnabled(enabled);
}

void MaemoRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    QFormLayout *formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QWidget *devConfWidget = new QWidget;
    QHBoxLayout *devConfLayout = new QHBoxLayout(devConfWidget);
    m_devConfLabel = new QLabel;
    devConfLayout->setMargin(0);
    devConfLayout->addWidget(m_devConfLabel);
    QLabel *addDevConfLabel= new QLabel(tr("<a href=\"%1\">Manage device configurations</a>")
        .arg(QLatin1String("deviceconfig")));
    addDevConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(addDevConfLabel);

    QLabel *debuggerConfLabel = new QLabel(tr("<a href=\"%1\">Set Debugger</a>")
        .arg(QLatin1String("debugger")));
    debuggerConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(debuggerConfLabel);

    formLayout->addRow(new QLabel(tr("Device configuration:")), devConfWidget);
    m_localExecutableLabel
        = new QLabel(m_runConfiguration->localExecutableFilePath());
    formLayout->addRow(tr("Executable on host:"), m_localExecutableLabel);
    m_remoteExecutableLabel = new QLabel;
    formLayout->addRow(tr("Executable on device:"), m_remoteExecutableLabel);
    m_argsLineEdit = new QLineEdit(m_runConfiguration->arguments());
    formLayout->addRow(tr("Arguments:"), m_argsLineEdit);

    QHBoxLayout * const debugButtonsLayout = new QHBoxLayout;
    m_debugCppOnlyButton = new QRadioButton(tr("C++ only"));
    m_debugQmlOnlyButton = new QRadioButton(tr("QML only"));
    m_debugCppAndQmlButton = new QRadioButton(tr("C++ and QML"));
    m_debuggingLanguagesLabel = new QLabel(tr("Debugging type:"));
    QButtonGroup * const buttonGroup = new QButtonGroup;
    buttonGroup->addButton(m_debugCppOnlyButton);
    buttonGroup->addButton(m_debugQmlOnlyButton);
    buttonGroup->addButton(m_debugCppAndQmlButton);
    debugButtonsLayout->addWidget(m_debugCppOnlyButton);
    debugButtonsLayout->addWidget(m_debugQmlOnlyButton);
    debugButtonsLayout->addWidget(m_debugCppAndQmlButton);
    formLayout->addRow(m_debuggingLanguagesLabel, debugButtonsLayout);
    if (m_runConfiguration->useCppDebugger()) {
        if (m_runConfiguration->useQmlDebugger())
            m_debugCppAndQmlButton->setChecked(true);
        else
            m_debugCppOnlyButton->setChecked(true);
    } else {
        m_debugQmlOnlyButton->setChecked(true);
    }

    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(debuggerConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(m_argsLineEdit, SIGNAL(textEdited(QString)), this,
        SLOT(argumentsEdited(QString)));
    connect(m_debugCppOnlyButton, SIGNAL(toggled(bool)), this,
        SLOT(handleDebuggingTypeChanged()));
    connect(m_debugQmlOnlyButton, SIGNAL(toggled(bool)), this,
        SLOT(handleDebuggingTypeChanged()));
    connect(m_debugCppAndQmlButton, SIGNAL(toggled(bool)), this,
        SLOT(handleDebuggingTypeChanged()));
    connect(m_runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(m_runConfiguration->target(),
        SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
        SLOT(handleActiveDeployConfigurationChanged()));
    handleActiveDeployConfigurationChanged();
}

void MaemoRunConfigurationWidget::addDebuggingWidgets(QVBoxLayout *mainLayout)
{
    m_debugDetailsContainer = new Utils::DetailsWidget(this);
    QWidget *debugWidget = new QWidget;
    m_debugDetailsContainer->setWidget(debugWidget);
    mainLayout->addWidget(m_debugDetailsContainer);
    QFormLayout *debugLayout = new QFormLayout(debugWidget);
    QHBoxLayout *debugRadioButtonsLayout = new QHBoxLayout;
    debugLayout->addRow(debugRadioButtonsLayout);
    QRadioButton *gdbButton = new QRadioButton(tr("Use remote GDB"));
    QRadioButton *gdbServerButton = new QRadioButton(tr("Use remote GDB server"));
    debugRadioButtonsLayout->addWidget(gdbButton);
    debugRadioButtonsLayout->addWidget(gdbServerButton);
    debugRadioButtonsLayout->addStretch(1);
    gdbButton->setChecked(m_runConfiguration->useRemoteGdb());
    gdbServerButton->setChecked(!gdbButton->isChecked());
    connect(gdbButton, SIGNAL(toggled(bool)), this,
        SLOT(handleDebuggingTypeChanged(bool)));
    handleDebuggingTypeChanged(gdbButton->isChecked());
}

void MaemoRunConfigurationWidget::addMountWidgets(QVBoxLayout *mainLayout)
{

    m_mountDetailsContainer = new Utils::DetailsWidget(this);
    QWidget *mountViewWidget = new QWidget;
    m_mountDetailsContainer->setWidget(mountViewWidget);
    mainLayout->addWidget(m_mountDetailsContainer);
    QVBoxLayout *mountViewLayout = new QVBoxLayout(mountViewWidget);
    m_mountWarningLabel = new QLabel;
    mountViewLayout->addWidget(m_mountWarningLabel);
    QHBoxLayout *tableLayout = new QHBoxLayout;
    mountViewLayout->addLayout(tableLayout);
    m_mountView = new QTableView;
    m_mountView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
    m_mountView->setSelectionBehavior(QTableView::SelectRows);
    m_mountView->setModel(m_runConfiguration->remoteMounts());
    tableLayout->addWidget(m_mountView);
    QVBoxLayout *mountViewButtonsLayout = new QVBoxLayout;
    tableLayout->addLayout(mountViewButtonsLayout);
    QToolButton *addMountButton = new QToolButton;
    QIcon plusIcon;
    plusIcon.addFile(QLatin1String(Core::Constants::ICON_PLUS));
    addMountButton->setIcon(plusIcon);
    mountViewButtonsLayout->addWidget(addMountButton);
    m_removeMountButton = new QToolButton;
    QIcon minusIcon;
    minusIcon.addFile(QLatin1String(Core::Constants::ICON_MINUS));
    m_removeMountButton->setIcon(minusIcon);
    mountViewButtonsLayout->addWidget(m_removeMountButton);
    mountViewButtonsLayout->addStretch(1);

    connect(addMountButton, SIGNAL(clicked()), this, SLOT(addMount()));
    connect(m_removeMountButton, SIGNAL(clicked()), this, SLOT(removeMount()));
    connect(m_mountView, SIGNAL(doubleClicked(QModelIndex)), this,
        SLOT(changeLocalMountDir(QModelIndex)));
    connect(m_mountView->selectionModel(),
        SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this,
        SLOT(enableOrDisableRemoveMountSpecButton()));
    connect(m_runConfiguration->remoteMounts(),
        SIGNAL(rowsInserted(QModelIndex, int, int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_runConfiguration->remoteMounts(),
        SIGNAL(rowsRemoved(QModelIndex, int, int)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_runConfiguration->remoteMounts(),
        SIGNAL(dataChanged(QModelIndex, QModelIndex)), this,
        SLOT(handleRemoteMountsChanged()));
    connect(m_runConfiguration->remoteMounts(), SIGNAL(modelReset()), this,
        SLOT(handleRemoteMountsChanged()));
    enableOrDisableRemoveMountSpecButton();
    handleRemoteMountsChanged();
}

void MaemoRunConfigurationWidget::addEnvironmentWidgets(QVBoxLayout *mainLayout)
{
    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList() << tr("Clean Environment")
        << tr("System Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(m_runConfiguration->baseEnvironmentBase());
    baseEnvironmentLayout->addWidget(m_baseEnvironmentComboBox);

    m_fetchEnv = new QPushButton(FetchEnvButtonText);
    baseEnvironmentLayout->addWidget(m_fetchEnv);
    baseEnvironmentLayout->addStretch(10);

    m_environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_environmentWidget->setBaseEnvironment(m_deviceEnvReader->deviceEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
    m_environmentWidget->setUserChanges(m_runConfiguration->userEnvironmentChanges());
    mainLayout->addWidget(m_environmentWidget);

    connect(m_environmentWidget, SIGNAL(userChangesChanged()), this,
        SLOT(userChangesEdited()));
    connect(m_baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(baseEnvironmentSelected(int)));
    connect(m_runConfiguration, SIGNAL(baseEnvironmentChanged()),
        this, SLOT(baseEnvironmentChanged()));
    connect(m_runConfiguration, SIGNAL(systemEnvironmentChanged()),
        this, SLOT(systemEnvironmentChanged()));
    connect(m_runConfiguration,
        SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
        this, SLOT(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)));
    connect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(m_deviceEnvReader, SIGNAL(finished()), this, SLOT(fetchEnvironmentFinished()));
    connect(m_deviceEnvReader, SIGNAL(error(QString)), this,
        SLOT(fetchEnvironmentError(QString)));
}

void MaemoRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_runConfiguration->setArguments(text);
}

void MaemoRunConfigurationWidget::updateTargetInformation()
{
    m_localExecutableLabel
        ->setText(QDir::toNativeSeparators(m_runConfiguration->localExecutableFilePath()));
}

void MaemoRunConfigurationWidget::handleActiveDeployConfigurationChanged()
{
    if (m_deployablesConnected)
        return;
    MaemoDeployStep * const deployStep = m_runConfiguration->deployStep();
    if (!deployStep)
        return;
    connect(deployStep->deployables().data(), SIGNAL(modelReset()),
        SLOT(handleDeploySpecsChanged()));
   handleDeploySpecsChanged();
   m_deployablesConnected = true;
   disconnect(m_runConfiguration->target(),
       SIGNAL(activeDeployConfigurationChanged(ProjectExplorer::DeployConfiguration*)),
       this,
       SLOT(handleActiveDeployConfigurationChanged()));

}

void MaemoRunConfigurationWidget::handleDeploySpecsChanged()
{
    m_remoteExecutableLabel->setText(m_runConfiguration->remoteExecutableFilePath());
    m_runConfiguration->updateFactoryState();
}

void MaemoRunConfigurationWidget::handleBuildConfigChanged()
{
    if (m_lastActiveBuildConfig)
        disconnect(m_lastActiveBuildConfig, 0, this, 0);
    m_lastActiveBuildConfig = m_runConfiguration->activeQt4BuildConfiguration();
    if (m_lastActiveBuildConfig) {
        connect(m_lastActiveBuildConfig, SIGNAL(qtVersionChanged()), this,
            SLOT(handleToolchainChanged()));
    }
    handleToolchainChanged();
}

void MaemoRunConfigurationWidget::handleToolchainChanged()
{
    const AbstractQt4MaemoTarget * const maemoTarget
        = m_runConfiguration->maemoTarget();
    if (maemoTarget) {
        const bool remoteMountsAvailable = maemoTarget->allowsRemoteMounts();
        m_debugDetailsContainer->setVisible(remoteMountsAvailable);
        m_mountDetailsContainer->setVisible(remoteMountsAvailable);
        const bool qmlDebuggingAvailable = maemoTarget->allowsQmlDebugging();
        m_debuggingLanguagesLabel->setVisible(qmlDebuggingAvailable);
        m_debugCppOnlyButton->setVisible(qmlDebuggingAvailable);
        m_debugQmlOnlyButton->setVisible(qmlDebuggingAvailable);
        m_debugCppAndQmlButton->setVisible(qmlDebuggingAvailable);
    }
    m_runConfiguration->updateFactoryState();
}

void MaemoRunConfigurationWidget::showDeviceConfigurationsDialog(const QString &link)
{
    if (link == QLatin1String("deviceconfig")) {
        MaemoDeviceConfigurationsSettingsPage *page
            = MaemoManager::instance().deviceConfigurationsSettingsPage();
        Core::ICore::instance()->showOptionsDialog(page->category(), page->id());
    } else if (link == QLatin1String("debugger")) {
        Core::ICore::instance()->showOptionsDialog(QLatin1String("O.Debugger"),
            QLatin1String("M.Gdb"));
    }
}

void MaemoRunConfigurationWidget::handleCurrentDeviceConfigChanged()
{
    m_devConfLabel->setText(MaemoGlobal::deviceConfigurationName(m_runConfiguration->deviceConfig()));
    updateMountWarning();
}

void MaemoRunConfigurationWidget::enableOrDisableRemoveMountSpecButton()
{
    const QModelIndexList selectedRows
        = m_mountView->selectionModel()->selectedRows();
    m_removeMountButton->setEnabled(!selectedRows.isEmpty());
}

void MaemoRunConfigurationWidget::addMount()
{
    const QString localDir = QFileDialog::getExistingDirectory(this,
        tr("Choose directory to mount"));
    if (!localDir.isEmpty()) {
        MaemoRemoteMountsModel * const mountsModel
            = m_runConfiguration->remoteMounts();
        mountsModel->addMountSpecification(localDir);
        m_mountView->edit(mountsModel->index(mountsModel->mountSpecificationCount() - 1,
            mountsModel->RemoteMountPointRow));
    }
}

void MaemoRunConfigurationWidget::removeMount()
{
    const QModelIndexList selectedRows
        = m_mountView->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        m_runConfiguration->remoteMounts()
            ->removeMountSpecificationAt(selectedRows.first().row());
    }
}

void MaemoRunConfigurationWidget::changeLocalMountDir(const QModelIndex &index)
{
    if (index.column() == MaemoRemoteMountsModel::LocalDirRow) {
        MaemoRemoteMountsModel * const mountsModel
            = m_runConfiguration->remoteMounts();
        const QString oldDir
            = mountsModel->mountSpecificationAt(index.row()).localDir;
        const QString localDir = QFileDialog::getExistingDirectory(this,
            tr("Choose directory to mount"), oldDir);
        if (!localDir.isEmpty())
            mountsModel->setLocalDir(index.row(), localDir);
    }
}

void MaemoRunConfigurationWidget::handleDebuggingTypeChanged(bool useGdb)
{
    m_runConfiguration->setUseRemoteGdb(useGdb);
    const QString detailsText = useGdb ?
                tr("<b>Debugging details:</b> Use GDB") :
                tr("<b>Debugging details:</b> Use GDB server");
    m_debugDetailsContainer->setSummaryText(detailsText);
}

void MaemoRunConfigurationWidget::fetchEnvironment()
{
    disconnect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    m_fetchEnv->setText(tr("Cancel Fetch Operation"));
    m_deviceEnvReader->start();
}

void MaemoRunConfigurationWidget::stopFetchEnvironment()
{
    m_deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

void MaemoRunConfigurationWidget::fetchEnvironmentFinished()
{
    disconnect(m_fetchEnv, SIGNAL(clicked()), this,
        SLOT(stopFetchEnvironment()));
    connect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    m_fetchEnv->setText(FetchEnvButtonText);
    m_runConfiguration->setSystemEnvironment(m_deviceEnvReader->deviceEnvironment());
}

void MaemoRunConfigurationWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device error"),
        tr("Fetching environment failed: %1").arg(error));
}

void MaemoRunConfigurationWidget::userChangesEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
    m_ignoreChange = false;
}

void MaemoRunConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseEnvironmentBase(MaemoRunConfiguration::BaseEnvironmentBase(index));

    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void MaemoRunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    m_baseEnvironmentComboBox->setCurrentIndex(m_runConfiguration->baseEnvironmentBase());
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
}

void MaemoRunConfigurationWidget::systemEnvironmentChanged()
{
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->systemEnvironment());
}

void MaemoRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setUserChanges(userChanges);
}

void MaemoRunConfigurationWidget::handleRemoteMountsChanged()
{
    const int mountCount
        = m_runConfiguration->remoteMounts()->validMountSpecificationCount();
    QString text;
    switch (mountCount) {
    case 0:
        text = tr("No local directories to be mounted on the device.");
        break;
    case 1:
        text = tr("One local directory to be mounted on the device.");
        break;
    default:
        //: Note: Only mountCount>1  will occur here as 0, 1 are handled above.
        text = tr("%n local directories to be mounted on the device.", 0, mountCount);
        break;
    }
    m_mountDetailsContainer->setSummaryText(QLatin1String("<b>") + text
        + QLatin1String("</b>"));
    updateMountWarning();
}

void MaemoRunConfigurationWidget::handleDebuggingTypeChanged()
{
    m_runConfiguration->setUseCppDebugger(m_debugCppOnlyButton->isChecked()
        || m_debugCppAndQmlButton->isChecked());
    m_runConfiguration->setUseQmlDebugger(m_debugQmlOnlyButton->isChecked()
        || m_debugCppAndQmlButton->isChecked());
    updateMountWarning();
}

void MaemoRunConfigurationWidget::updateMountWarning()
{
    QString mountWarning;
    const MaemoPortList &portList = m_runConfiguration->freePorts();
    const int availablePortCount = portList.count();
    const int mountDirCount
            = m_runConfiguration->remoteMounts()->validMountSpecificationCount();
    if (mountDirCount > availablePortCount) {
        mountWarning = tr("WARNING: You want to mount %1 directories, but "
            "your device has only %n free ports.<br>You will not be able "
            "to run this configuration.", 0, availablePortCount)
                .arg(mountDirCount);
    } else if (mountDirCount > 0) {
        const int portsLeftByDebuggers = availablePortCount
                - m_runConfiguration->portsUsedByDebuggers();
        if (mountDirCount > portsLeftByDebuggers) {
            mountWarning = tr("WARNING: You want to mount %1 directories, "
                "but only %n ports on the device will be available "
                "in debug mode. <br>You will not be able to debug your "
                "application with this configuration.", 0, portsLeftByDebuggers)
                    .arg(mountDirCount);
        }
    }
    if (mountWarning.isEmpty()) {
        m_mountWarningLabel->hide();
    } else {
        m_mountWarningLabel->setText(QLatin1String("<font color=\"red\">")
            + mountWarning + QLatin1String("</font>"));
        m_mountWarningLabel->show();
        m_mountDetailsContainer->setState(Utils::DetailsWidget::Expanded);
    }
    m_runConfiguration->updateFactoryState();
}

} // namespace Internal
} // namespace Qt4ProjectManager
