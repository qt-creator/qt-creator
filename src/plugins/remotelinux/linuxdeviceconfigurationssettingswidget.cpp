/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include <QFileInfo>
#include <QRegExp>
#include <QSettings>
#include <QSignalMapper>
#include <QTextStream>

#include <QFileDialog>
#include <QMessageBox>
#include <QIntValidator>

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
      m_ui(new Ui::LinuxDeviceConfigurationsSettingsWidget),
      m_devConfigs(LinuxDeviceConfigurations::cloneInstance()),
      m_nameValidator(new NameValidator(m_devConfigs.data(), this)),
      m_saveSettingsRequested(false),
      m_additionalActionsMapper(new QSignalMapper(this)),
      m_configWidget(0)
{
    LinuxDeviceConfigurations::blockCloning();
    initGui();
    connect(m_additionalActionsMapper, SIGNAL(mapped(QString)),
        SLOT(handleAdditionalActionRequest(QString)));
}

LinuxDeviceConfigurationsSettingsWidget::~LinuxDeviceConfigurationsSettingsWidget()
{
    if (m_saveSettingsRequested) {
        Core::ICore::settings()->setValue(LastDeviceConfigIndexKey,
            currentIndex());
        LinuxDeviceConfigurations::replaceInstance(m_devConfigs.data());
    }
    LinuxDeviceConfigurations::unblockCloning();
    delete m_ui;
}

QString LinuxDeviceConfigurationsSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->configurationLabel->text()
        << ' ' << m_ui->deviceTypeLabel->text()
        << ' ' << m_ui->deviceTypeValueLabel->text()
        << ' ' << m_ui->deviceNameLabel->text()
        << ' ' << m_ui->nameLineEdit->text();
    if (m_configWidget)
    rc.remove(QLatin1Char('&'));
    return rc;
}

void LinuxDeviceConfigurationsSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->configurationComboBox->setModel(m_devConfigs.data());
    m_ui->nameLineEdit->setValidator(m_nameValidator);

    int lastIndex = Core::ICore::settings()
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

    if (current->deviceType() == LinuxDeviceConfiguration::Hardware)
        m_ui->deviceTypeValueLabel->setText(tr("Physical Device"));
    else
        m_ui->deviceTypeValueLabel->setText(tr("Emulator"));
    m_nameValidator->setDisplayName(current->displayName());
    m_ui->removeConfigButton->setEnabled(!current->isAutoDetected());
    fillInValues();
}

void LinuxDeviceConfigurationsSettingsWidget::fillInValues()
{
    const LinuxDeviceConfiguration::ConstPtr &current = currentConfig();
    m_ui->nameLineEdit->setText(current->displayName());
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

void LinuxDeviceConfigurationsSettingsWidget::setDefaultDevice()
{
    m_devConfigs->setDefaultDevice(currentIndex());
    m_ui->defaultDeviceButton->setEnabled(false);
}

void LinuxDeviceConfigurationsSettingsWidget::showGenerateSshKeyDialog()
{
    SshKeyCreationDialog dialog(this);
    dialog.exec();
}

void LinuxDeviceConfigurationsSettingsWidget::currentConfigChanged(int index)
{
    qDeleteAll(m_additionalActionButtons);
    delete m_configWidget;
    m_configWidget = 0;
    m_additionalActionButtons.clear();
    m_ui->generalGroupBox->setEnabled(false);
    m_ui->osSpecificGroupBox->setEnabled(false);
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
                QPushButton * const button = new QPushButton(
                            factory->displayNameForActionId(actionId));
                m_additionalActionButtons << button;
                connect(button, SIGNAL(clicked()), m_additionalActionsMapper, SLOT(map()));
                m_additionalActionsMapper->setMapping(button, actionId);
                m_ui->buttonsLayout->insertWidget(m_ui->buttonsLayout->count() - 1, button);
            }
            if (!m_ui->osSpecificGroupBox->layout())
                new QVBoxLayout(m_ui->osSpecificGroupBox);
            m_configWidget = factory->createWidget(m_devConfigs->mutableDeviceAt(currentIndex()),
                                                   m_ui->osSpecificGroupBox);
            if (m_configWidget) {
                connect(m_configWidget, SIGNAL(defaultSshKeyFilePathChanged(QString)),
                        m_devConfigs.data(), SLOT(setDefaultSshKeyFilePath(QString)));
                m_ui->osSpecificGroupBox->layout()->addWidget(m_configWidget);
                m_ui->osSpecificGroupBox->setEnabled(factory->isUserEditable());
            }
            m_ui->generalGroupBox->setEnabled(factory->isUserEditable());
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
    if (action)
        action->exec();
    delete action;
}

} // namespace Internal
} // namespace RemoteLinux
