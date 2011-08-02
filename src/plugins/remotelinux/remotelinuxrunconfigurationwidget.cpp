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

#include "linuxdeviceconfiguration.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxenvironmentreader.h"
#include "remotelinuxsettingspages.h"
#include "remotelinuxutils.h"

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
namespace Internal {
namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxRunConfigurationWidget",
          "Fetch Device Environment");
} // anonymous namespace

class RemoteLinuxRunConfigurationWidgetPrivate
{
public:
    RemoteLinuxRunConfigurationWidgetPrivate(RemoteLinuxRunConfiguration *runConfig)
        : runConfiguration(runConfig), deviceEnvReader(runConfiguration), ignoreChange(false)
    {
    }

    RemoteLinuxRunConfiguration * const runConfiguration;
    RemoteLinuxEnvironmentReader deviceEnvReader;
    bool ignoreChange;

    QWidget topWidget;
    QLabel disabledIcon;
    QLabel disabledReason;
    QLineEdit argsLineEdit;
    QLabel localExecutableLabel;
    QLabel remoteExecutableLabel;
    QLabel devConfLabel;
    QLabel debuggingLanguagesLabel;
    QRadioButton debugCppOnlyButton;
    QRadioButton debugQmlOnlyButton;
    QRadioButton debugCppAndQmlButton;
    QPushButton fetchEnvButton;
    QComboBox baseEnvironmentComboBox;
    ProjectExplorer::EnvironmentWidget *environmentWidget;
};

} // namespace Internal

using namespace Internal;

RemoteLinuxRunConfigurationWidget::RemoteLinuxRunConfigurationWidget(RemoteLinuxRunConfiguration *runConfiguration,
        QWidget *parent)
    : QWidget(parent), m_d(new RemoteLinuxRunConfigurationWidgetPrivate(runConfiguration))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    addDisabledLabel(topLayout);
    topLayout->addWidget(&m_d->topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(&m_d->topWidget);
    mainLayout->setMargin(0);
    addGenericWidgets(mainLayout);
    addEnvironmentWidgets(mainLayout);

    connect(m_d->runConfiguration, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        SLOT(handleCurrentDeviceConfigChanged()));
    handleCurrentDeviceConfigChanged();
    connect(m_d->runConfiguration, SIGNAL(isEnabledChanged(bool)),
        SLOT(runConfigurationEnabledChange(bool)));
    runConfigurationEnabledChange(m_d->runConfiguration->isEnabled());
}

RemoteLinuxRunConfigurationWidget::~RemoteLinuxRunConfigurationWidget()
{
    delete m_d;
}

void RemoteLinuxRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout * const hl = new QHBoxLayout;
    hl->addStretch();
    m_d->disabledIcon.setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(&m_d->disabledIcon);
    m_d->disabledReason.setVisible(false);
    hl->addWidget(&m_d->disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void RemoteLinuxRunConfigurationWidget::suppressQmlDebuggingOptions()
{
    m_d->debuggingLanguagesLabel.hide();
    m_d->debugCppOnlyButton.hide();
    m_d->debugQmlOnlyButton.hide();
    m_d->debugCppAndQmlButton.hide();
}

void RemoteLinuxRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    m_d->topWidget.setEnabled(enabled);
    m_d->disabledIcon.setVisible(!enabled);
    m_d->disabledReason.setVisible(!enabled);
    m_d->disabledReason.setText(m_d->runConfiguration->disabledReason());
}

void RemoteLinuxRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    QFormLayout * const formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QWidget * const devConfWidget = new QWidget;
    QHBoxLayout * const devConfLayout = new QHBoxLayout(devConfWidget);
    devConfLayout->setMargin(0);
    devConfLayout->addWidget(&m_d->devConfLabel);
    QLabel * const addDevConfLabel= new QLabel(tr("<a href=\"%1\">Manage device configurations</a>")
        .arg(QLatin1String("deviceconfig")));
    addDevConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(addDevConfLabel);

    QLabel * const debuggerConfLabel = new QLabel(tr("<a href=\"%1\">Set Debugger</a>")
        .arg(QLatin1String("debugger")));
    debuggerConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(debuggerConfLabel);

    formLayout->addRow(new QLabel(tr("Device configuration:")), devConfWidget);
    m_d->localExecutableLabel.setText(m_d->runConfiguration->localExecutableFilePath());
    formLayout->addRow(tr("Executable on host:"), &m_d->localExecutableLabel);
    formLayout->addRow(tr("Executable on device:"), &m_d->remoteExecutableLabel);
    m_d->argsLineEdit.setText(m_d->runConfiguration->arguments());
    formLayout->addRow(tr("Arguments:"), &m_d->argsLineEdit);

    QHBoxLayout * const debugButtonsLayout = new QHBoxLayout;
    m_d->debugCppOnlyButton.setText(tr("C++ only"));
    m_d->debugQmlOnlyButton.setText(tr("QML only"));
    m_d->debugCppAndQmlButton.setText(tr("C++ and QML"));
    m_d->debuggingLanguagesLabel.setText(tr("Debugging type:"));
    QButtonGroup * const buttonGroup = new QButtonGroup;
    buttonGroup->addButton(&m_d->debugCppOnlyButton);
    buttonGroup->addButton(&m_d->debugQmlOnlyButton);
    buttonGroup->addButton(&m_d->debugCppAndQmlButton);
    debugButtonsLayout->addWidget(&m_d->debugCppOnlyButton);
    debugButtonsLayout->addWidget(&m_d->debugQmlOnlyButton);
    debugButtonsLayout->addWidget(&m_d->debugCppAndQmlButton);
    debugButtonsLayout->addStretch(1);
    formLayout->addRow(&m_d->debuggingLanguagesLabel, debugButtonsLayout);
    if (m_d->runConfiguration->useCppDebugger()) {
        if (m_d->runConfiguration->useQmlDebugger())
            m_d->debugCppAndQmlButton.setChecked(true);
        else
            m_d->debugCppOnlyButton.setChecked(true);
    } else {
        m_d->debugQmlOnlyButton.setChecked(true);
    }

    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(debuggerConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(&m_d->argsLineEdit, SIGNAL(textEdited(QString)), SLOT(argumentsEdited(QString)));
    connect(&m_d->debugCppOnlyButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(&m_d->debugQmlOnlyButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(&m_d->debugCppAndQmlButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(m_d->runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(m_d->runConfiguration, SIGNAL(deploySpecsChanged()), SLOT(handleDeploySpecsChanged()));
    handleDeploySpecsChanged();
}

void RemoteLinuxRunConfigurationWidget::addEnvironmentWidgets(QVBoxLayout *mainLayout)
{
    QWidget * const baseEnvironmentWidget = new QWidget;
    QHBoxLayout * const baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel * const label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseEnvironmentLayout->addWidget(label);
    m_d->baseEnvironmentComboBox.addItems(QStringList() << tr("Clean Environment")
        << tr("System Environment"));
    m_d->baseEnvironmentComboBox.setCurrentIndex(m_d->runConfiguration->baseEnvironmentType());
    baseEnvironmentLayout->addWidget(&m_d->baseEnvironmentComboBox);

    m_d->fetchEnvButton.setText(FetchEnvButtonText);
    baseEnvironmentLayout->addWidget(&m_d->fetchEnvButton);
    baseEnvironmentLayout->addStretch(10);

    m_d->environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    m_d->environmentWidget->setBaseEnvironment(m_d->deviceEnvReader.deviceEnvironment());
    m_d->environmentWidget->setBaseEnvironmentText(m_d->runConfiguration->baseEnvironmentText());
    m_d->environmentWidget->setUserChanges(m_d->runConfiguration->userEnvironmentChanges());
    mainLayout->addWidget(m_d->environmentWidget);

    connect(m_d->environmentWidget, SIGNAL(userChangesChanged()), SLOT(userChangesEdited()));
    connect(&m_d->baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(baseEnvironmentSelected(int)));
    connect(m_d->runConfiguration, SIGNAL(baseEnvironmentChanged()),
        this, SLOT(baseEnvironmentChanged()));
    connect(m_d->runConfiguration, SIGNAL(systemEnvironmentChanged()),
        this, SLOT(systemEnvironmentChanged()));
    connect(m_d->runConfiguration,
        SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
        SLOT(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)));
    connect(&m_d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&m_d->deviceEnvReader, SIGNAL(finished()), this, SLOT(fetchEnvironmentFinished()));
    connect(&m_d->deviceEnvReader, SIGNAL(error(QString)), SLOT(fetchEnvironmentError(QString)));
}

void RemoteLinuxRunConfigurationWidget::argumentsEdited(const QString &text)
{
    m_d->runConfiguration->setArguments(text);
}

void RemoteLinuxRunConfigurationWidget::updateTargetInformation()
{
    m_d->localExecutableLabel
        .setText(QDir::toNativeSeparators(m_d->runConfiguration->localExecutableFilePath()));
}

void RemoteLinuxRunConfigurationWidget::handleDeploySpecsChanged()
{
    m_d->remoteExecutableLabel.setText(m_d->runConfiguration->remoteExecutableFilePath());
}

void RemoteLinuxRunConfigurationWidget::showDeviceConfigurationsDialog(const QString &link)
{
    if (link == QLatin1String("deviceconfig")) {
        Core::ICore::instance()->showOptionsDialog(LinuxDeviceConfigurationsSettingsPage::pageCategory(),
            LinuxDeviceConfigurationsSettingsPage::pageId());
    } else if (link == QLatin1String("debugger")) {
        Core::ICore::instance()->showOptionsDialog(QLatin1String("O.Debugger"),
            QLatin1String("M.Gdb"));
    }
}

void RemoteLinuxRunConfigurationWidget::handleCurrentDeviceConfigChanged()
{
    m_d->devConfLabel.setText(RemoteLinuxUtils::deviceConfigurationName(m_d->runConfiguration->deviceConfig()));
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironment()
{
    disconnect(&m_d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&m_d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    m_d->fetchEnvButton.setText(tr("Cancel Fetch Operation"));
    m_d->deviceEnvReader.start(m_d->runConfiguration->environmentPreparationCommand());
}

void RemoteLinuxRunConfigurationWidget::stopFetchEnvironment()
{
    m_d->deviceEnvReader.stop();
    fetchEnvironmentFinished();
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentFinished()
{
    disconnect(&m_d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    connect(&m_d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    m_d->fetchEnvButton.setText(FetchEnvButtonText);
    m_d->runConfiguration->setSystemEnvironment(m_d->deviceEnvReader.deviceEnvironment());
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxRunConfigurationWidget::userChangesEdited()
{
    m_d->ignoreChange = true;
    m_d->runConfiguration->setUserEnvironmentChanges(m_d->environmentWidget->userChanges());
    m_d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentSelected(int index)
{
    m_d->ignoreChange = true;
    m_d->runConfiguration->setBaseEnvironmentType(RemoteLinuxRunConfiguration::BaseEnvironmentType(index));
    m_d->environmentWidget->setBaseEnvironment(m_d->runConfiguration->baseEnvironment());
    m_d->environmentWidget->setBaseEnvironmentText(m_d->runConfiguration->baseEnvironmentText());
    m_d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentChanged()
{
    if (m_d->ignoreChange)
        return;

    m_d->baseEnvironmentComboBox.setCurrentIndex(m_d->runConfiguration->baseEnvironmentType());
    m_d->environmentWidget->setBaseEnvironment(m_d->runConfiguration->baseEnvironment());
    m_d->environmentWidget->setBaseEnvironmentText(m_d->runConfiguration->baseEnvironmentText());
}

void RemoteLinuxRunConfigurationWidget::systemEnvironmentChanged()
{
    m_d->environmentWidget->setBaseEnvironment(m_d->runConfiguration->systemEnvironment());
}

void RemoteLinuxRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (m_d->ignoreChange)
        return;
    m_d->environmentWidget->setUserChanges(userChanges);
}

void RemoteLinuxRunConfigurationWidget::handleDebuggingTypeChanged()
{
    m_d->runConfiguration->setUseCppDebugger(m_d->debugCppOnlyButton.isChecked()
        || m_d->debugCppAndQmlButton.isChecked());
    m_d->runConfiguration->setUseQmlDebugger(m_d->debugQmlOnlyButton.isChecked()
        || m_d->debugCppAndQmlButton.isChecked());
}

} // namespace RemoteLinux
