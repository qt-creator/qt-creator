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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "remotelinuxrunconfigurationwidget.h"

#include "maemodeviceenvreader.h"
#include "maemoglobal.h"
#include "remotelinuxrunconfiguration.h"
#include "maemosettingspages.h"
#include "qt4maemodeployconfiguration.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <qt4projectmanager/qt4buildconfiguration.h>
#include <qt4projectmanager/qt4target.h>
#include <utils/detailswidget.h>

#include <QtGui/QButtonGroup>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtGui/QComboBox>
#include <QtGui/QFormLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>

using namespace Qt4ProjectManager;

namespace RemoteLinux {
namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxRunConfigurationWidget",
          "Fetch Device Environment");
} // anonymous namespace

using namespace Internal;

RemoteLinuxRunConfigurationWidget::RemoteLinuxRunConfigurationWidget(RemoteLinuxRunConfiguration *runConfiguration,
        QWidget *parent)
    : QWidget(parent),
    m_runConfiguration(runConfiguration),
    m_ignoreChange(false),
    m_deviceEnvReader(new MaemoDeviceEnvReader(this, runConfiguration))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    addDisabledLabel(topLayout);
    topWidget = new QWidget;
    topLayout->addWidget(topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(topWidget);
    mainLayout->setMargin(0);
    addGenericWidgets(mainLayout);
    addEnvironmentWidgets(mainLayout);
    connect(m_runConfiguration,
        SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        this, SLOT(handleCurrentDeviceConfigChanged()));
    handleCurrentDeviceConfigChanged();

    connect(m_runConfiguration, SIGNAL(isEnabledChanged(bool)),
            this, SLOT(runConfigurationEnabledChange(bool)));
    runConfigurationEnabledChange(m_runConfiguration->isEnabled());
}

void RemoteLinuxRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout *hl = new QHBoxLayout;
    hl->addStretch();
    m_disabledIcon = new QLabel(this);
    m_disabledIcon->setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(m_disabledIcon);
    m_disabledReason = new QLabel(this);
    m_disabledReason->setVisible(false);
    hl->addWidget(m_disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void RemoteLinuxRunConfigurationWidget::suppressQmlDebuggingOptions()
{
    m_debuggingLanguagesLabel->hide();
    m_debugCppOnlyButton->hide();
    m_debugQmlOnlyButton->hide();
    m_debugCppAndQmlButton->hide();
}

void RemoteLinuxRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{    topWidget->setEnabled(enabled);
    m_disabledIcon->setVisible(!enabled);
    m_disabledReason->setVisible(!enabled);
    m_disabledReason->setText(m_runConfiguration->disabledReason());
}

void RemoteLinuxRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
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
    debugButtonsLayout->addStretch(1);
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
    connect(m_runConfiguration, SIGNAL(deploySpecsChanged()), SLOT(handleDeploySpecsChanged()));
    handleDeploySpecsChanged();
}

void RemoteLinuxRunConfigurationWidget::addEnvironmentWidgets(QVBoxLayout *mainLayout)
{
    QWidget *baseEnvironmentWidget = new QWidget;
    QHBoxLayout *baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel *label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_baseEnvironmentComboBox = new QComboBox(this);
    m_baseEnvironmentComboBox->addItems(QStringList() << tr("Clean Environment")
        << tr("System Environment"));
    m_baseEnvironmentComboBox->setCurrentIndex(m_runConfiguration->baseEnvironmentType());
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

void RemoteLinuxRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_runConfiguration->setArguments(text);
}

void RemoteLinuxRunConfigurationWidget::updateTargetInformation()
{
    m_localExecutableLabel
        ->setText(QDir::toNativeSeparators(m_runConfiguration->localExecutableFilePath()));
}

void RemoteLinuxRunConfigurationWidget::handleDeploySpecsChanged()
{
    m_remoteExecutableLabel->setText(m_runConfiguration->remoteExecutableFilePath());
}

void RemoteLinuxRunConfigurationWidget::showDeviceConfigurationsDialog(const QString &link)
{
    if (link == QLatin1String("deviceconfig")) {
        Core::ICore::instance()->showOptionsDialog(MaemoDeviceConfigurationsSettingsPage::Category,
            MaemoDeviceConfigurationsSettingsPage::Id);
    } else if (link == QLatin1String("debugger")) {
        Core::ICore::instance()->showOptionsDialog(QLatin1String("O.Debugger"),
            QLatin1String("M.Gdb"));
    }
}

void RemoteLinuxRunConfigurationWidget::handleCurrentDeviceConfigChanged()
{
    m_devConfLabel->setText(MaemoGlobal::deviceConfigurationName(m_runConfiguration->deviceConfig()));
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironment()
{
    disconnect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    m_fetchEnv->setText(tr("Cancel Fetch Operation"));
    m_deviceEnvReader->start();
}

void RemoteLinuxRunConfigurationWidget::stopFetchEnvironment()
{
    m_deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentFinished()
{
    disconnect(m_fetchEnv, SIGNAL(clicked()), this,
        SLOT(stopFetchEnvironment()));
    connect(m_fetchEnv, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    m_fetchEnv->setText(FetchEnvButtonText);
    m_runConfiguration->setSystemEnvironment(m_deviceEnvReader->deviceEnvironment());
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxRunConfigurationWidget::userChangesEdited()
{
    m_ignoreChange = true;
    m_runConfiguration->setUserEnvironmentChanges(m_environmentWidget->userChanges());
    m_ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_ignoreChange = true;
    m_runConfiguration->setBaseEnvironmentType(RemoteLinuxRunConfiguration::BaseEnvironmentType(index));

    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
    m_ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_ignoreChange)
        return;

    m_baseEnvironmentComboBox->setCurrentIndex(m_runConfiguration->baseEnvironmentType());
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->baseEnvironment());
    m_environmentWidget->setBaseEnvironmentText(m_runConfiguration->baseEnvironmentText());
}

void RemoteLinuxRunConfigurationWidget::systemEnvironmentChanged()
{
    m_environmentWidget->setBaseEnvironment(m_runConfiguration->systemEnvironment());
}

void RemoteLinuxRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (m_ignoreChange)
        return;
    m_environmentWidget->setUserChanges(userChanges);
}

void RemoteLinuxRunConfigurationWidget::handleDebuggingTypeChanged()
{
    m_runConfiguration->setUseCppDebugger(m_debugCppOnlyButton->isChecked()
        || m_debugCppAndQmlButton->isChecked());
    m_runConfiguration->setUseQmlDebugger(m_debugQmlOnlyButton->isChecked()
        || m_debugCppAndQmlButton->isChecked());
}

} // namespace RemoteLinux
