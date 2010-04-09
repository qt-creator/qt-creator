/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of Qt Creator.
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

#include "maemosettingswidget.h"

#include "ui_maemosettingswidget.h"

#include "maemoconfigtestdialog.h"
#include "maemodeviceconfigurations.h"
#include "maemosshthread.h"

#include <QtCore/QRegExp>
#include <QtCore/QFileInfo>
#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>

#include <algorithm>

namespace Qt4ProjectManager {
namespace Internal {

bool configNameExists(const QList<MaemoDeviceConfig> &devConfs,
                      const QString &name)
{
    return std::find_if(devConfs.constBegin(), devConfs.constEnd(),
        DevConfNameMatcher(name)) != devConfs.constEnd();
}

class TimeoutValidator : public QIntValidator
{
public:
    TimeoutValidator() : QIntValidator(0, SHRT_MAX, 0)
    {
    }

    void setValue(int oldValue) { m_oldValue = oldValue; }

    virtual void fixup(QString &input) const
    {
        int dummy = 0;
        if (validate(input, dummy) != Acceptable)
            input = QString::number(m_oldValue);
    }

private:
    int m_oldValue;
};

class NameValidator : public QValidator
{
public:
    NameValidator(const QList<MaemoDeviceConfig> &devConfs)
        : m_devConfs(devConfs)
    {
    }

    void setDisplayName(const QString &name) { m_oldName = name; }

    virtual State validate(QString &input, int & /* pos */) const
    {
        if (input.trimmed().isEmpty()
            || (input != m_oldName && configNameExists(m_devConfs, input)))
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
    const QList<MaemoDeviceConfig> &m_devConfs;
};


MaemoSettingsWidget::MaemoSettingsWidget(QWidget *parent)
    : QWidget(parent),
      m_ui(new Ui_MaemoSettingsWidget),
      m_devConfs(MaemoDeviceConfigurations::instance().devConfigs()),
      m_nameValidator(new NameValidator(m_devConfs)),
      m_timeoutValidator(new TimeoutValidator),
      m_keyDeployer(0)

{
    initGui();
}

MaemoSettingsWidget::~MaemoSettingsWidget()
{
}

void MaemoSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->nameLineEdit->setValidator(m_nameValidator);
    m_ui->timeoutLineEdit->setValidator(m_timeoutValidator);
    m_ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    foreach (const MaemoDeviceConfig &devConf, m_devConfs)
        m_ui->configurationComboBox->addItem(devConf.name);
    connect(m_ui->configurationComboBox, SIGNAL(currentIndexChanged(int)), SLOT(currentConfigChanged(int)));
    currentConfigChanged(m_ui->configurationComboBox->currentIndex());
}

void MaemoSettingsWidget::addConfig()
{
    const QString prefix = tr("New Device Configuration %1", "Standard Configuration name with number");
    int suffix = 1;
    QString newName;
    bool isUnique = false;
    do {
        newName = prefix.arg(QString::number(suffix++));
        isUnique = !configNameExists(m_devConfs, newName);
    } while (!isUnique);

    m_devConfs.append(MaemoDeviceConfig(newName, MaemoDeviceConfig::Physical));
    m_ui->configurationComboBox->addItem(newName);
    m_ui->removeConfigButton->setEnabled(true);
    m_ui->configurationComboBox->setCurrentIndex(m_ui->configurationComboBox->count()-1);
    m_ui->configurationComboBox->setFocus();
}

void MaemoSettingsWidget::deleteConfig()
{
    const int selectedItem = m_ui->configurationComboBox->currentIndex();
    m_devConfs.removeAt(selectedItem);
    m_ui->configurationComboBox->removeItem(selectedItem);
    Q_ASSERT(m_ui->configurationComboBox->count() == m_devConfs.count());
}

void MaemoSettingsWidget::display(const MaemoDeviceConfig &devConfig)
{
    MaemoDeviceConfig *otherConfig;
    if (devConfig.type == MaemoDeviceConfig::Physical) {
        m_lastConfigHW = devConfig;
        m_lastConfigSim
            = MaemoDeviceConfig(devConfig.name, MaemoDeviceConfig::Simulator);
        otherConfig = &m_lastConfigSim;
        m_ui->deviceButton->setChecked(true);
    } else {
        m_lastConfigSim = devConfig;
        m_lastConfigHW
            = MaemoDeviceConfig(devConfig.name, MaemoDeviceConfig::Physical);
        otherConfig = &m_lastConfigHW;
        m_ui->simulatorButton->setChecked(true);
    }
    otherConfig->authentication = devConfig.authentication;
    otherConfig->timeout = devConfig.timeout;
    otherConfig->pwd = devConfig.pwd;
    otherConfig->keyFile = devConfig.keyFile;

    if (devConfig.authentication == MaemoDeviceConfig::Password)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->detailsWidget->setEnabled(true);
    m_nameValidator->setDisplayName(devConfig.name);
    m_timeoutValidator->setValue(devConfig.timeout);
    fillInValues();
}

void MaemoSettingsWidget::fillInValues()
{
    m_ui->nameLineEdit->setText(currentConfig().name);
    m_ui->hostLineEdit->setText(currentConfig().host);
    m_ui->sshPortSpinBox->setValue(currentConfig().sshPort);
    m_ui->gdbServerPortSpinBox->setValue(currentConfig().gdbServerPort);
    m_ui->timeoutLineEdit->setText(QString::number(currentConfig().timeout));
    m_ui->userLineEdit->setText(currentConfig().uname);
    m_ui->pwdLineEdit->setText(currentConfig().pwd);
    m_ui->keyFileLineEdit->setPath(currentConfig().keyFile);

    const bool isSimulator
        = currentConfig().type == MaemoDeviceConfig::Simulator;
    m_ui->hostLineEdit->setReadOnly(isSimulator);
    m_ui->sshPortSpinBox->setReadOnly(isSimulator);
    m_ui->gdbServerPortSpinBox->setReadOnly(isSimulator);
}

void MaemoSettingsWidget::saveSettings()
{
    MaemoDeviceConfigurations::instance().setDevConfigs(m_devConfs);
}

MaemoDeviceConfig &MaemoSettingsWidget::currentConfig()
{
    Q_ASSERT(m_ui->configurationComboBox->count() == m_devConfs.count());
    const int currenIndex = m_ui->configurationComboBox->currentIndex();
    Q_ASSERT(currenIndex != -1);
    Q_ASSERT(currenIndex < m_devConfs.count());
    return m_devConfs[currenIndex];
}

void MaemoSettingsWidget::configNameEditingFinished()
{
    if (m_ui->configurationComboBox->count() == 0)
        return;

    const QString &newName = m_ui->nameLineEdit->text();
    const int currentIndex = m_ui->configurationComboBox->currentIndex();
    m_ui->configurationComboBox->setItemData(currentIndex, newName, Qt::DisplayRole);
    currentConfig().name = newName;
    m_nameValidator->setDisplayName(newName);
}

void MaemoSettingsWidget::deviceTypeChanged()
{
    const MaemoDeviceConfig::DeviceType devType =
        m_ui->deviceButton->isChecked()
            ? MaemoDeviceConfig::Physical
            : MaemoDeviceConfig::Simulator;

    if (devType == MaemoDeviceConfig::Simulator) {
        m_lastConfigHW = currentConfig();
        currentConfig() = m_lastConfigSim;
    } else {
        m_lastConfigSim = currentConfig();
        currentConfig() = m_lastConfigHW;
    }
    fillInValues();
}

void MaemoSettingsWidget::authenticationTypeChanged()
{
    const bool usePassword = m_ui->passwordButton->isChecked();
    currentConfig().authentication = usePassword
        ? MaemoDeviceConfig::Password
        : MaemoDeviceConfig::Key;
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
    m_ui->deployKeyButton->setEnabled(usePassword);
}

void MaemoSettingsWidget::hostNameEditingFinished()
{
    currentConfig().host = m_ui->hostLineEdit->text();
}

void MaemoSettingsWidget::sshPortEditingFinished()
{
    currentConfig().sshPort = m_ui->sshPortSpinBox->value();
}

void MaemoSettingsWidget::gdbServerPortEditingFinished()
{
    currentConfig().gdbServerPort = m_ui->gdbServerPortSpinBox->value();
}

void MaemoSettingsWidget::timeoutEditingFinished()
{
    setTimeout(m_ui->timeoutLineEdit, currentConfig().timeout,
                     m_timeoutValidator);
}

void MaemoSettingsWidget::setTimeout(const QLineEdit *lineEdit,
    int &confVal, TimeoutValidator *validator)
{
    bool ok;
    confVal = lineEdit->text().toInt(&ok);
    Q_ASSERT(ok);
    validator->setValue(confVal);
}

void MaemoSettingsWidget::userNameEditingFinished()
{
    currentConfig().uname = m_ui->userLineEdit->text();
}

void MaemoSettingsWidget::passwordEditingFinished()
{
    currentConfig().pwd = m_ui->pwdLineEdit->text();
}

void MaemoSettingsWidget::keyFileEditingFinished()
{
    currentConfig().keyFile = m_ui->keyFileLineEdit->path();
}

void MaemoSettingsWidget::testConfig()
{
    QDialog *dialog = new MaemoConfigTestDialog(currentConfig(), this);
    dialog->open();
}

void MaemoSettingsWidget::deployKey()
{
    if (m_keyDeployer)
        return;

    const QString &dir = QFileInfo(currentConfig().keyFile).path();
    const QString &keyFile
        = QFileDialog::getOpenFileName(this, tr("Choose public key file"), dir);
    if (keyFile.isEmpty())
        return;

    m_ui->deployKeyButton->disconnect();
    SshDeploySpec deploySpec(keyFile,
                             homeDirOnDevice(currentConfig().uname)
                                 + QLatin1String("/.ssh/authorized_keys"),
                             true);
    m_keyDeployer = new MaemoSshDeployer(currentConfig(),
                                         QList<SshDeploySpec>() << deploySpec);
    connect(m_keyDeployer, SIGNAL(finished()),
            this, SLOT(handleDeployThreadFinished()));
    m_ui->deployKeyButton->setText(tr("Stop deploying"));
    connect(m_ui->deployKeyButton, SIGNAL(clicked()),
            this, SLOT(stopDeploying()));
    m_keyDeployer->start();
}

void MaemoSettingsWidget::handleDeployThreadFinished()
{
    if (!m_keyDeployer)
        return;

    if (m_keyDeployer->hasError())
        QMessageBox::critical(this, tr("Deployment Failed"), tr("Key deployment failed: %1").arg(m_keyDeployer->error()));
    else
        QMessageBox::information(this, tr("Deployment Succeeded"), tr("Key was successfully deployed."));

    stopDeploying();
}

void MaemoSettingsWidget::stopDeploying()
{
    if (m_keyDeployer) {
        m_ui->deployKeyButton->disconnect();
        const bool buttonWasEnabled = m_ui->deployKeyButton->isEnabled();
        m_keyDeployer->disconnect();
        m_keyDeployer->stop();
        delete m_keyDeployer;
        m_keyDeployer = 0;
        m_ui->deployKeyButton->setText(tr("Deploy Key ..."));
        connect(m_ui->deployKeyButton, SIGNAL(clicked()),
                this, SLOT(deployKey()));
        m_ui->deployKeyButton->setEnabled(buttonWasEnabled);
    }
}

void MaemoSettingsWidget::currentConfigChanged(int index)
{
    stopDeploying();
    if (index == -1) {
        m_ui->removeConfigButton->setEnabled(false);
        m_ui->testConfigButton->setEnabled(false);
        clearDetails();
        m_ui->detailsWidget->setEnabled(false);
    } else {
        m_ui->removeConfigButton->setEnabled(true);
        m_ui->testConfigButton->setEnabled(true);
        m_ui->configurationComboBox->setCurrentIndex(index);
        display(currentConfig());
    }
}

void MaemoSettingsWidget::clearDetails()
{
    m_ui->hostLineEdit->clear();
    m_ui->sshPortSpinBox->clear();
    m_ui->gdbServerPortSpinBox->clear();
    m_ui->timeoutLineEdit->clear();
    m_ui->userLineEdit->clear();
    m_ui->pwdLineEdit->clear();
}

} // namespace Internal
} // namespace Qt4ProjectManager
