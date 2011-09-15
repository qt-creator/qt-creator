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
    : QWidget(parent), d(new RemoteLinuxRunConfigurationWidgetPrivate(runConfiguration))
{
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    topLayout->setMargin(0);
    addDisabledLabel(topLayout);
    topLayout->addWidget(&d->topWidget);
    QVBoxLayout *mainLayout = new QVBoxLayout(&d->topWidget);
    mainLayout->setMargin(0);
    addGenericWidgets(mainLayout);
    addEnvironmentWidgets(mainLayout);

    connect(d->runConfiguration, SIGNAL(deviceConfigurationChanged(ProjectExplorer::Target*)),
        SLOT(handleCurrentDeviceConfigChanged()));
    handleCurrentDeviceConfigChanged();
    connect(d->runConfiguration, SIGNAL(isEnabledChanged(bool)),
        SLOT(runConfigurationEnabledChange(bool)));
    runConfigurationEnabledChange(d->runConfiguration->isEnabled());
}

RemoteLinuxRunConfigurationWidget::~RemoteLinuxRunConfigurationWidget()
{
    delete d;
}

void RemoteLinuxRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout * const hl = new QHBoxLayout;
    hl->addStretch();
    d->disabledIcon.setPixmap(QPixmap(QString::fromUtf8(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(&d->disabledIcon);
    d->disabledReason.setVisible(false);
    hl->addWidget(&d->disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void RemoteLinuxRunConfigurationWidget::suppressQmlDebuggingOptions()
{
    d->debuggingLanguagesLabel.hide();
    d->debugCppOnlyButton.hide();
    d->debugQmlOnlyButton.hide();
    d->debugCppAndQmlButton.hide();
}

void RemoteLinuxRunConfigurationWidget::runConfigurationEnabledChange(bool enabled)
{
    d->topWidget.setEnabled(enabled);
    d->disabledIcon.setVisible(!enabled);
    d->disabledReason.setVisible(!enabled);
    d->disabledReason.setText(d->runConfiguration->disabledReason());
}

void RemoteLinuxRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    QFormLayout * const formLayout = new QFormLayout;
    mainLayout->addLayout(formLayout);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QWidget * const devConfWidget = new QWidget;
    QHBoxLayout * const devConfLayout = new QHBoxLayout(devConfWidget);
    devConfLayout->setMargin(0);
    devConfLayout->addWidget(&d->devConfLabel);
    QLabel * const addDevConfLabel= new QLabel(tr("<a href=\"%1\">Manage device configurations</a>")
        .arg(QLatin1String("deviceconfig")));
    addDevConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(addDevConfLabel);

    QLabel * const debuggerConfLabel = new QLabel(tr("<a href=\"%1\">Set Debugger</a>")
        .arg(QLatin1String("debugger")));
    debuggerConfLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    devConfLayout->addWidget(debuggerConfLabel);

    formLayout->addRow(new QLabel(tr("Device configuration:")), devConfWidget);
    d->localExecutableLabel.setText(d->runConfiguration->localExecutableFilePath());
    formLayout->addRow(tr("Executable on host:"), &d->localExecutableLabel);
    formLayout->addRow(tr("Executable on device:"), &d->remoteExecutableLabel);
    d->argsLineEdit.setText(d->runConfiguration->arguments());
    formLayout->addRow(tr("Arguments:"), &d->argsLineEdit);

    QHBoxLayout * const debugButtonsLayout = new QHBoxLayout;
    d->debugCppOnlyButton.setText(tr("C++ only"));
    d->debugQmlOnlyButton.setText(tr("QML only"));
    d->debugCppAndQmlButton.setText(tr("C++ and QML"));
    d->debuggingLanguagesLabel.setText(tr("Debugging type:"));
    QButtonGroup * const buttonGroup = new QButtonGroup;
    buttonGroup->addButton(&d->debugCppOnlyButton);
    buttonGroup->addButton(&d->debugQmlOnlyButton);
    buttonGroup->addButton(&d->debugCppAndQmlButton);
    debugButtonsLayout->addWidget(&d->debugCppOnlyButton);
    debugButtonsLayout->addWidget(&d->debugQmlOnlyButton);
    debugButtonsLayout->addWidget(&d->debugCppAndQmlButton);
    debugButtonsLayout->addStretch(1);
    formLayout->addRow(&d->debuggingLanguagesLabel, debugButtonsLayout);
    if (d->runConfiguration->useCppDebugger()) {
        if (d->runConfiguration->useQmlDebugger())
            d->debugCppAndQmlButton.setChecked(true);
        else
            d->debugCppOnlyButton.setChecked(true);
    } else {
        d->debugQmlOnlyButton.setChecked(true);
    }

    connect(addDevConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(debuggerConfLabel, SIGNAL(linkActivated(QString)), this,
        SLOT(showDeviceConfigurationsDialog(QString)));
    connect(&d->argsLineEdit, SIGNAL(textEdited(QString)), SLOT(argumentsEdited(QString)));
    connect(&d->debugCppOnlyButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(&d->debugQmlOnlyButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(&d->debugCppAndQmlButton, SIGNAL(toggled(bool)), SLOT(handleDebuggingTypeChanged()));
    connect(d->runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(d->runConfiguration, SIGNAL(deploySpecsChanged()), SLOT(handleDeploySpecsChanged()));
    handleDeploySpecsChanged();
}

void RemoteLinuxRunConfigurationWidget::addEnvironmentWidgets(QVBoxLayout *mainLayout)
{
    QWidget * const baseEnvironmentWidget = new QWidget;
    QHBoxLayout * const baseEnvironmentLayout = new QHBoxLayout(baseEnvironmentWidget);
    baseEnvironmentLayout->setMargin(0);
    QLabel * const label = new QLabel(tr("Base environment for this run configuration:"), this);
    baseEnvironmentLayout->addWidget(label);
    d->baseEnvironmentComboBox.addItems(QStringList() << tr("Clean Environment")
        << tr("System Environment"));
    d->baseEnvironmentComboBox.setCurrentIndex(d->runConfiguration->baseEnvironmentType());
    baseEnvironmentLayout->addWidget(&d->baseEnvironmentComboBox);

    d->fetchEnvButton.setText(FetchEnvButtonText);
    baseEnvironmentLayout->addWidget(&d->fetchEnvButton);
    baseEnvironmentLayout->addStretch(10);

    d->environmentWidget = new ProjectExplorer::EnvironmentWidget(this, baseEnvironmentWidget);
    d->environmentWidget->setBaseEnvironment(d->deviceEnvReader.deviceEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
    d->environmentWidget->setUserChanges(d->runConfiguration->userEnvironmentChanges());
    mainLayout->addWidget(d->environmentWidget);

    connect(d->environmentWidget, SIGNAL(userChangesChanged()), SLOT(userChangesEdited()));
    connect(&d->baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(baseEnvironmentSelected(int)));
    connect(d->runConfiguration, SIGNAL(baseEnvironmentChanged()),
        this, SLOT(baseEnvironmentChanged()));
    connect(d->runConfiguration, SIGNAL(systemEnvironmentChanged()),
        this, SLOT(systemEnvironmentChanged()));
    connect(d->runConfiguration,
        SIGNAL(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)),
        SLOT(userEnvironmentChangesChanged(QList<Utils::EnvironmentItem>)));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&d->deviceEnvReader, SIGNAL(finished()), this, SLOT(fetchEnvironmentFinished()));
    connect(&d->deviceEnvReader, SIGNAL(error(QString)), SLOT(fetchEnvironmentError(QString)));
}

void RemoteLinuxRunConfigurationWidget::argumentsEdited(const QString &text)
{
    d->runConfiguration->setArguments(text);
}

void RemoteLinuxRunConfigurationWidget::updateTargetInformation()
{
    d->localExecutableLabel
        .setText(QDir::toNativeSeparators(d->runConfiguration->localExecutableFilePath()));
}

void RemoteLinuxRunConfigurationWidget::handleDeploySpecsChanged()
{
    d->remoteExecutableLabel.setText(d->runConfiguration->remoteExecutableFilePath());
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
    d->devConfLabel.setText(RemoteLinuxUtils::deviceConfigurationName(d->runConfiguration->deviceConfig()));
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironment()
{
    disconnect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    d->fetchEnvButton.setText(tr("Cancel Fetch Operation"));
    d->deviceEnvReader.start(d->runConfiguration->environmentPreparationCommand());
}

void RemoteLinuxRunConfigurationWidget::stopFetchEnvironment()
{
    d->deviceEnvReader.stop();
    fetchEnvironmentFinished();
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentFinished()
{
    disconnect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(stopFetchEnvironment()));
    connect(&d->fetchEnvButton, SIGNAL(clicked()), this, SLOT(fetchEnvironment()));
    d->fetchEnvButton.setText(FetchEnvButtonText);
    d->runConfiguration->setSystemEnvironment(d->deviceEnvReader.deviceEnvironment());
}

void RemoteLinuxRunConfigurationWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxRunConfigurationWidget::userChangesEdited()
{
    d->ignoreChange = true;
    d->runConfiguration->setUserEnvironmentChanges(d->environmentWidget->userChanges());
    d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentSelected(int index)
{
    d->ignoreChange = true;
    d->runConfiguration->setBaseEnvironmentType(RemoteLinuxRunConfiguration::BaseEnvironmentType(index));
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->baseEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
    d->ignoreChange = false;
}

void RemoteLinuxRunConfigurationWidget::baseEnvironmentChanged()
{
    if (d->ignoreChange)
        return;

    d->baseEnvironmentComboBox.setCurrentIndex(d->runConfiguration->baseEnvironmentType());
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->baseEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
}

void RemoteLinuxRunConfigurationWidget::systemEnvironmentChanged()
{
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->systemEnvironment());
}

void RemoteLinuxRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (d->ignoreChange)
        return;
    d->environmentWidget->setUserChanges(userChanges);
}

void RemoteLinuxRunConfigurationWidget::handleDebuggingTypeChanged()
{
    d->runConfiguration->setUseCppDebugger(d->debugCppOnlyButton.isChecked()
        || d->debugCppAndQmlButton.isChecked());
    d->runConfiguration->setUseQmlDebugger(d->debugQmlOnlyButton.isChecked()
        || d->debugCppAndQmlButton.isChecked());
}

} // namespace RemoteLinux
