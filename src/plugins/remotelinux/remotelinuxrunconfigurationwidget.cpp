/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "remotelinuxrunconfigurationwidget.h"

#include "remotelinuxrunconfiguration.h"
#include "remotelinuxenvironmentreader.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/detailswidget.h>

#include <QCoreApplication>
#include <QDir>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

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
    QLineEdit workingDirLineEdit;
    QLabel localExecutableLabel;
    QLabel remoteExecutableLabel;
    QCheckBox useAlternateCommandBox;
    QLineEdit alternateCommand;
    QLabel devConfLabel;
    QPushButton fetchEnvButton;
    QComboBox baseEnvironmentComboBox;
    ProjectExplorer::EnvironmentWidget *environmentWidget;
    QFormLayout genericWidgetsLayout;
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

    connect(d->runConfiguration, SIGNAL(enabledChanged()),
        SLOT(runConfigurationEnabledChange()));
    runConfigurationEnabledChange();
}

RemoteLinuxRunConfigurationWidget::~RemoteLinuxRunConfigurationWidget()
{
    delete d;
}

void RemoteLinuxRunConfigurationWidget::addFormLayoutRow(QWidget *label, QWidget *field)
{
    d->genericWidgetsLayout.addRow(label, field);
}

void RemoteLinuxRunConfigurationWidget::addDisabledLabel(QVBoxLayout *topLayout)
{
    QHBoxLayout * const hl = new QHBoxLayout;
    hl->addStretch();
    d->disabledIcon.setPixmap(QPixmap(QLatin1String(":/projectexplorer/images/compile_warning.png")));
    hl->addWidget(&d->disabledIcon);
    d->disabledReason.setVisible(false);
    hl->addWidget(&d->disabledReason);
    hl->addStretch();
    topLayout->addLayout(hl);
}

void RemoteLinuxRunConfigurationWidget::runConfigurationEnabledChange()
{
    bool enabled = d->runConfiguration->isEnabled();
    d->topWidget.setEnabled(enabled);
    d->disabledIcon.setVisible(!enabled);
    d->disabledReason.setVisible(!enabled);
    d->disabledReason.setText(d->runConfiguration->disabledReason());
}

void RemoteLinuxRunConfigurationWidget::addGenericWidgets(QVBoxLayout *mainLayout)
{
    Utils::DetailsWidget *detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);

    QWidget *details = new QWidget(this);
    details->setLayout(&d->genericWidgetsLayout);
    detailsContainer->setWidget(details);

    mainLayout->addWidget(detailsContainer);

    d->genericWidgetsLayout.setFormAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    d->localExecutableLabel.setText(d->runConfiguration->localExecutableFilePath());
    d->genericWidgetsLayout.addRow(tr("Executable on host:"), &d->localExecutableLabel);
    d->genericWidgetsLayout.addRow(tr("Executable on device:"), &d->remoteExecutableLabel);
    QWidget * const altRemoteExeWidget = new QWidget;
    QHBoxLayout * const altRemoteExeLayout = new QHBoxLayout(altRemoteExeWidget);
    altRemoteExeLayout->setContentsMargins(0, 0, 0, 0);
    d->alternateCommand.setText(d->runConfiguration->alternateRemoteExecutable());
    altRemoteExeLayout->addWidget(&d->alternateCommand);
    d->useAlternateCommandBox.setText(tr("Use this command instead"));
    d->useAlternateCommandBox.setChecked(d->runConfiguration->useAlternateExecutable());
    altRemoteExeLayout->addWidget(&d->useAlternateCommandBox);
    d->genericWidgetsLayout.addRow(tr("Alternate executable on device:"), altRemoteExeWidget);

    d->argsLineEdit.setText(d->runConfiguration->arguments());
    d->genericWidgetsLayout.addRow(tr("Arguments:"), &d->argsLineEdit);

    d->workingDirLineEdit.setPlaceholderText(tr("<default>"));
    d->workingDirLineEdit.setText(d->runConfiguration->workingDirectory());
    d->genericWidgetsLayout.addRow(tr("Working directory:"), &d->workingDirLineEdit);

    connect(&d->argsLineEdit, SIGNAL(textEdited(QString)), SLOT(argumentsEdited(QString)));
    connect(d->runConfiguration, SIGNAL(targetInformationChanged()), this,
        SLOT(updateTargetInformation()));
    connect(d->runConfiguration, SIGNAL(deploySpecsChanged()), SLOT(handleDeploySpecsChanged()));
    connect(&d->useAlternateCommandBox, SIGNAL(toggled(bool)),
        SLOT(handleUseAlternateCommandChanged()));
    connect(&d->alternateCommand, SIGNAL(textEdited(QString)),
        SLOT(handleAlternateCommandChanged()));
    connect(&d->workingDirLineEdit, SIGNAL(textEdited(QString)),
        SLOT(handleWorkingDirectoryChanged()));
    handleDeploySpecsChanged();
    handleUseAlternateCommandChanged();
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
    d->environmentWidget->setBaseEnvironment(d->deviceEnvReader.remoteEnvironment());
    d->environmentWidget->setBaseEnvironmentText(d->runConfiguration->baseEnvironmentText());
    d->environmentWidget->setUserChanges(d->runConfiguration->userEnvironmentChanges());
    mainLayout->addWidget(d->environmentWidget);

    connect(d->environmentWidget, SIGNAL(userChangesChanged()), SLOT(userChangesEdited()));
    connect(&d->baseEnvironmentComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(baseEnvironmentSelected(int)));
    connect(d->runConfiguration, SIGNAL(baseEnvironmentChanged()),
        this, SLOT(baseEnvironmentChanged()));
    connect(d->runConfiguration, SIGNAL(remoteEnvironmentChanged()),
        this, SLOT(remoteEnvironmentChanged()));
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
    setLabelText(d->localExecutableLabel,
            QDir::toNativeSeparators(d->runConfiguration->localExecutableFilePath()),
            tr("Unknown"));
}

void RemoteLinuxRunConfigurationWidget::handleDeploySpecsChanged()
{
    setLabelText(d->remoteExecutableLabel, d->runConfiguration->defaultRemoteExecutableFilePath(),
            tr("Remote path not set"));
}

void RemoteLinuxRunConfigurationWidget::setLabelText(QLabel &label, const QString &regularText,
        const QString &errorText)
{
    const QString errorMessage = QLatin1String("<font color=\"red\">") + errorText
            + QLatin1String("</font>");
    label.setText(regularText.isEmpty() ? errorMessage : regularText);
}

void RemoteLinuxRunConfigurationWidget::handleUseAlternateCommandChanged()
{
    const bool useAltExe = d->useAlternateCommandBox.isChecked();
    d->remoteExecutableLabel.setEnabled(!useAltExe);
    d->alternateCommand.setEnabled(useAltExe);
    d->runConfiguration->setUseAlternateExecutable(useAltExe);
}

void RemoteLinuxRunConfigurationWidget::handleAlternateCommandChanged()
{
    d->runConfiguration->setAlternateRemoteExecutable(d->alternateCommand.text().trimmed());
}

void RemoteLinuxRunConfigurationWidget::handleWorkingDirectoryChanged()
{
    d->runConfiguration->setWorkingDirectory(d->workingDirLineEdit.text().trimmed());
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
    d->runConfiguration->setRemoteEnvironment(d->deviceEnvReader.remoteEnvironment());
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

void RemoteLinuxRunConfigurationWidget::remoteEnvironmentChanged()
{
    d->environmentWidget->setBaseEnvironment(d->runConfiguration->remoteEnvironment());
}

void RemoteLinuxRunConfigurationWidget::userEnvironmentChangesChanged(const QList<Utils::EnvironmentItem> &userChanges)
{
    if (d->ignoreChange)
        return;
    d->environmentWidget->setUserChanges(userChanges);
}

} // namespace RemoteLinux
