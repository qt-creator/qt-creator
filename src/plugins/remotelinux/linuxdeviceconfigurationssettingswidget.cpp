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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/
#include "linuxdeviceconfigurationssettingswidget.h"

#include "ui_linuxdeviceconfigurationssettingswidget.h"

#include "linuxdeviceconfigurations.h"
#include "linuxdevicefactoryselectiondialog.h"
#include "portlist.h"
#include "remotelinuxutils.h"
#include "sshkeycreationdialog.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QSettings>
#include <QtCore/QSignalMapper>
#include <QtCore/QTextStream>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>

#include <algorithm>

using namespace Core;
using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
const QLatin1String LastDeviceConfigIndexKey("LastDisplayedMaemoDeviceConfig");
} // anonymous namespace


class NameValidator : public QValidator
{
public:
    NameValidator(const LinuxDeviceConfigurations *devConfigs,
            QWidget *parent = 0)
        : QValidator(parent), m_devConfigs(devConfigs)
    {
    }

    void setDisplayName(const QString &name) { m_oldName = name; }

    virtual State validate(QString &input, int & /* pos */) const
    {
        if (input.trimmed().isEmpty()
                || (input != m_oldName && m_devConfigs->hasConfig(input)))
            return Intermediate;
        return Acceptable;
    }

    virtual void fixup(QString &input) const
    {
        int dummy = 0;
        if (validate(input, dummy) != Acceptable)
            input = m_oldName;
    }

private:
    QString m_oldName;
    const LinuxDeviceConfigurations * const m_devConfigs;
};


LinuxDeviceConfigurationsSettingsWidget::LinuxDeviceConfigurationsSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_LinuxDeviceConfigurationsSettingsWidget),
      m_devConfigs(LinuxDeviceConfigurations::cloneInstance()),
      m_nameValidator(new NameValidator(m_devConfigs.data(), this)),
      m_saveSettingsRequested(false),
      m_additionalActionsMapper(new QSignalMapper(this))
{
    initGui();
    connect(m_additionalActionsMapper, SIGNAL(mapped(QString)),
        SLOT(handleAdditionalActionRequest(QString)));
}

LinuxDeviceConfigurationsSettingsWidget::~LinuxDeviceConfigurationsSettingsWidget()
{
    if (m_saveSettingsRequested) {
        Core::ICore::instance()->settings()->setValue(LastDeviceConfigIndexKey,
            currentIndex());
        LinuxDeviceConfigurations::replaceInstance(m_devConfigs.data());
    }
    delete m_ui;
}

QString LinuxDeviceConfigurationsSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->configurationLabel->text()
        << ' ' << m_ui->sshPortLabel->text()
        << ' ' << m_ui->keyButton->text()
        << ' ' << m_ui->passwordButton->text()
        << ' ' << m_ui->authTypeLabel->text()
        << ' ' << m_ui->connectionTimeoutLabel->text()
        << ' ' << m_ui->deviceTypeLabel->text()
        << ' ' << m_ui->deviceTypeValueLabel->text()
        << ' ' << m_ui->deviceNameLabel->text()
        << ' ' << m_ui->hostNameLabel->text()
        << ' ' << m_ui->keyLabel->text()
        << ' ' << m_ui->nameLineEdit->text()
        << ' ' << m_ui->passwordLabel->text()
        << ' ' << m_ui->freePortsLabel->text()
        << ' ' << m_ui->pwdLineEdit->text()
        << ' ' << m_ui->timeoutSpinBox->value()
        << ' ' << m_ui->userLineEdit->text()
        << ' ' << m_ui->userNameLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void LinuxDeviceConfigurationsSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->portsWarningLabel->setPixmap(QPixmap(":/projectexplorer/images/compile_error.png"));
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
        + tr("You will need at least one port.") + QLatin1String("</font>"));
    m_ui->configurationComboBox->setModel(m_devConfigs.data());
    m_ui->nameLineEdit->setValidator(m_nameValidator);
    m_ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    m_ui->keyFileLineEdit->lineEdit()->setMinimumWidth(0);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(PortList::regularExpression()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    connect(m_ui->makeKeyFileDefaultButton, SIGNAL(clicked()),
        SLOT(setDefaultKeyFilePath()));
    int lastIndex = Core::ICore::instance()->settings()
        ->value(LastDeviceConfigIndexKey, 0).toInt();
    if (lastIndex == -1)
        lastIndex = 0;
    if (lastIndex < m_ui->configurationComboBox->count())
        m_ui->configurationComboBox->setCurrentIndex(lastIndex);
    connect(m_ui->configurationComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(currentConfigChanged(int)));
    currentConfigChanged(currentIndex());
    connect(m_ui->defaultDeviceButton, SIGNAL(clicked()),
        SLOT(setDefaultDevice()));
}

void LinuxDeviceConfigurationsSettingsWidget::addConfig()
{
    const QList<ILinuxDeviceConfigurationFactory *> &factories
        = ExtensionSystem::PluginManager::instance()->getObjects<ILinuxDeviceConfigurationFactory>();

    if (factories.isEmpty()) // Can't happen, because this plugin provides the generic one.
        return;

    LinuxDeviceFactorySelectionDialog d;
    if (d.exec() != QDialog::Accepted)
        return;

    const QScopedPointer<ILinuxDeviceConfigurationWizard> wizard(d.selectedFactory()->createWizard(this));
    if (wizard->exec() != QDialog::Accepted)
        return;

    m_devConfigs->addConfiguration(wizard->deviceConfiguration());
    m_ui->removeConfigButton->setEnabled(true);
    m_ui->configurationComboBox->setCurrentIndex(m_ui->configurationComboBox->count()-1);
}

void LinuxDeviceConfigurationsSettingsWidget::deleteConfig()
{
    m_devConfigs->removeConfiguration(currentIndex());
    if (m_devConfigs->rowCount() == 0)
        currentConfigChanged(-1);
}

void LinuxDeviceConfigurationsSettingsWidget::displayCurrent()
{
    const LinuxDeviceConfiguration::ConstPtr &current = currentConfig();
    m_ui->defaultDeviceButton->setEnabled(!current->isDefault());
    m_ui->osTypeValueLabel->setText(RemoteLinuxUtils::osTypeToString(current->osType()));
    const SshConnectionParameters &sshParams = current->sshParameters();
    if (current->deviceType() == LinuxDeviceConfiguration::Hardware)
        m_ui->deviceTypeValueLabel->setText(tr("Physical Device"));
    else
        m_ui->deviceTypeValueLabel->setText(tr("Emulator"));
    if (sshParams.authenticationType == Utils::SshConnectionParameters::AuthenticationByPassword)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_nameValidator->setDisplayName(current->name());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    fillInValues();
}

void LinuxDeviceConfigurationsSettingsWidget::fillInValues()
{
    const LinuxDeviceConfiguration::ConstPtr &current = currentConfig();
    m_ui->nameLineEdit->setText(current->name());
    const SshConnectionParameters &sshParams = current->sshParameters();
    m_ui->hostLineEdit->setText(sshParams.host);
    m_ui->sshPortSpinBox->setValue(sshParams.port);
    m_ui->portsLineEdit->setText(current->freePorts().toString());
    m_ui->timeoutSpinBox->setValue(sshParams.timeout);
    m_ui->userLineEdit->setText(sshParams.userName);
    m_ui->pwdLineEdit->setText(sshParams.password);
    m_ui->keyFileLineEdit->setPath(sshParams.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    updatePortsWarningLabel();
}

void LinuxDeviceConfigurationsSettingsWidget::saveSettings()
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    m_saveSettingsRequested = true;
}

int LinuxDeviceConfigurationsSettingsWidget::currentIndex() const
{
    return m_ui->configurationComboBox->currentIndex();
}

LinuxDeviceConfiguration::ConstPtr LinuxDeviceConfigurationsSettingsWidget::currentConfig() const
{
    Q_ASSERT(currentIndex() != -1);
    return m_devConfigs->deviceAt(currentIndex());
}

void LinuxDeviceConfigurationsSettingsWidget::configNameEditingFinished()
{
    if (m_ui->configurationComboBox->count() == 0)
        return;

    const QString &newName = m_ui->nameLineEdit->text();
    m_devConfigs->setConfigurationName(currentIndex(), newName);
    m_nameValidator->setDisplayName(newName);
}

void LinuxDeviceConfigurationsSettingsWidget::authenticationTypeChanged()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    const bool usePassword = m_ui->passwordButton->isChecked();
    sshParams.authenticationType = usePassword
        ? SshConnectionParameters::AuthenticationByPassword
        : SshConnectionParameters::AuthenticationByKey;
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
    m_ui->makeKeyFileDefaultButton->setEnabled(!usePassword);
}

void LinuxDeviceConfigurationsSettingsWidget::hostNameEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.host = m_ui->hostLineEdit->text();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::sshPortEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.port = m_ui->sshPortSpinBox->value();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::timeoutEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.timeout = m_ui->timeoutSpinBox->value();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::userNameEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.userName = m_ui->userLineEdit->text();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::passwordEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.password = m_ui->pwdLineEdit->text();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::keyFileEditingFinished()
{
    SshConnectionParameters sshParams = currentConfig()->sshParameters();
    sshParams.privateKeyFile = m_ui->keyFileLineEdit->path();
    m_devConfigs->setSshParameters(currentIndex(), sshParams);
}

void LinuxDeviceConfigurationsSettingsWidget::handleFreePortsChanged()
{
    m_devConfigs->setFreePorts(currentIndex(), PortList::fromString(m_ui->portsLineEdit->text()));
    updatePortsWarningLabel();
}

void LinuxDeviceConfigurationsSettingsWidget::showPassword(bool showClearText)
{
    m_ui->pwdLineEdit->setEchoMode(showClearText
        ? QLineEdit::Normal : QLineEdit::Password);
}

void LinuxDeviceConfigurationsSettingsWidget::showGenerateSshKeyDialog()
{
    SshKeyCreationDialog dialog(this);
    dialog.exec();
}

void LinuxDeviceConfigurationsSettingsWidget::setDefaultKeyFilePath()
{
    m_devConfigs->setDefaultSshKeyFilePath(m_ui->keyFileLineEdit->path());
}

void LinuxDeviceConfigurationsSettingsWidget::setDefaultDevice()
{
    m_devConfigs->setDefaultDevice(currentIndex());
    m_ui->defaultDeviceButton->setEnabled(false);
}

void LinuxDeviceConfigurationsSettingsWidget::setPrivateKey(const QString &path)
{
    m_ui->keyFileLineEdit->setPath(path);
    keyFileEditingFinished();
}

void LinuxDeviceConfigurationsSettingsWidget::currentConfigChanged(int index)
{
    qDeleteAll(m_additionalActionButtons);
    m_additionalActionButtons.clear();
    m_ui->detailsWidget->setEnabled(false);
    if (index == -1) {
        m_ui->removeConfigButton->setEnabled(false);
        m_ui->generateKeyButton->setEnabled(false);
        clearDetails();
        m_ui->defaultDeviceButton->setEnabled(false);
    } else {
        m_ui->removeConfigButton->setEnabled(true);
        m_ui->generateKeyButton->setEnabled(true);
        const ILinuxDeviceConfigurationFactory * const factory = factoryForCurrentConfig();
        if (factory) {
            const QStringList &actionIds = factory->supportedDeviceActionIds();
            foreach (const QString &actionId, actionIds) {
                QPushButton * const button = new QPushButton(factory->displayNameForActionId(actionId));
                m_additionalActionButtons << button;
                connect(button, SIGNAL(clicked()), m_additionalActionsMapper, SLOT(map()));
                m_additionalActionsMapper->setMapping(button, actionId);
                m_ui->buttonsLayout->insertWidget(m_ui->buttonsLayout->count() - 1, button);
            }
            m_ui->detailsWidget->setEnabled(factory->isUserEditable());
        }
        m_ui->configurationComboBox->setCurrentIndex(index);
        displayCurrent();
    }
}

void LinuxDeviceConfigurationsSettingsWidget::clearDetails()
{
    m_ui->nameLineEdit->clear();
    m_ui->osTypeValueLabel->clear();
    m_ui->deviceTypeValueLabel->clear();
    m_ui->hostLineEdit->clear();
    m_ui->sshPortSpinBox->clear();
    m_ui->timeoutSpinBox->clear();
    m_ui->userLineEdit->clear();
    m_ui->pwdLineEdit->clear();
    m_ui->portsLineEdit->clear();
    m_ui->portsWarningLabel->clear();
    m_ui->keyFileLineEdit->lineEdit()->clear();
}

void LinuxDeviceConfigurationsSettingsWidget::updatePortsWarningLabel()
{
    m_ui->portsWarningLabel->setVisible(!currentConfig()->freePorts().hasMore());
}

const ILinuxDeviceConfigurationFactory *LinuxDeviceConfigurationsSettingsWidget::factoryForCurrentConfig() const
{
    Q_ASSERT(currentConfig());
    const QList<ILinuxDeviceConfigurationFactory *> &factories
        = ExtensionSystem::PluginManager::instance()->getObjects<ILinuxDeviceConfigurationFactory>();
    foreach (const ILinuxDeviceConfigurationFactory * const factory, factories) {
        if (factory->supportsOsType(currentConfig()->osType()))
            return factory;
    }
    return 0;
}

void LinuxDeviceConfigurationsSettingsWidget::handleAdditionalActionRequest(const QString &actionId)
{
    const ILinuxDeviceConfigurationFactory * const factory = factoryForCurrentConfig();
    Q_ASSERT(factory);
    QDialog * const action = factory->createDeviceAction(actionId, currentConfig(), this);
    Q_ASSERT(action);
    action->exec();
    delete action;
}

} // namespace Internal
} // namespace RemoteLinux
