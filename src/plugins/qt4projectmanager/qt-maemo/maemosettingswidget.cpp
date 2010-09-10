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
#include "maemosshconfigdialog.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegExp>
#include <QtCore/QTextStream>

#include <QtGui/QFileDialog>
#include <QtGui/QMessageBox>
#include <QtGui/QIntValidator>

#include <algorithm>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

bool configNameExists(const QList<MaemoDeviceConfig> &devConfs,
                      const QString &name)
{
    return std::find_if(devConfs.constBegin(), devConfs.constEnd(),
        DevConfNameMatcher(name)) != devConfs.constEnd();
}

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
      m_saveSettingsRequested(false)
{
    initGui();
}

MaemoSettingsWidget::~MaemoSettingsWidget()
{
    if (m_saveSettingsRequested)
        MaemoDeviceConfigurations::instance().setDevConfigs(m_devConfs);
}

QString MaemoSettingsWidget::searchKeywords() const
{
    QString rc;
    QTextStream(&rc) << m_ui->configurationLabel->text()
        << ' ' << m_ui->sshPortLabel->text()
        << ' ' << m_ui->keyButton->text()
        << ' ' << m_ui->passwordButton->text()
        << ' ' << m_ui->authTypeLabel->text()
        << ' ' << m_ui->connectionTimeoutLabel->text()
        << ' ' << m_ui->deviceButton->text()
        << ' ' << m_ui->simulatorButton->text()
        << ' ' << m_ui->deviceTypeLabel->text()
        << ' ' << m_ui->deviceNameLabel->text()
        << ' ' << m_ui->hostNameLabel->text()
        << ' ' << m_ui->keyLabel->text()
        << ' ' << m_ui->nameLineEdit->text()
        << ' ' << m_ui->passwordLabel->text()
        << ' ' << m_ui->freePortsLabel->text()
        << ' ' << m_ui->portsWarningLabel->text()
        << ' ' << m_ui->pwdLineEdit->text()
        << ' ' << m_ui->timeoutSpinBox->value()
        << ' ' << m_ui->userLineEdit->text()
        << ' ' << m_ui->userNameLabel->text();
    rc.remove(QLatin1Char('&'));
    return rc;
}

void MaemoSettingsWidget::initGui()
{
    m_ui->setupUi(this);
    m_ui->nameLineEdit->setValidator(m_nameValidator);
    m_ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    QRegExpValidator * const portsValidator
        = new QRegExpValidator(QRegExp(MaemoDeviceConfig::portsRegExpr()), this);
    m_ui->portsLineEdit->setValidator(portsValidator);
    foreach (const MaemoDeviceConfig &devConf, m_devConfs)
        m_ui->configurationComboBox->addItem(devConf.name);
    connect(m_ui->configurationComboBox, SIGNAL(currentIndexChanged(int)),
        SLOT(currentConfigChanged(int)));
    currentConfigChanged(m_ui->configurationComboBox->currentIndex());
}

void MaemoSettingsWidget::addConfig()
{
    const QString prefix = tr("New Device Configuration %1", "Standard "
        "Configuration name with number");
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
    otherConfig->server.authType = devConfig.server.authType;
    otherConfig->server.timeout = devConfig.server.timeout;
    otherConfig->server.pwd = devConfig.server.pwd;
    otherConfig->server.privateKeyFile = devConfig.server.privateKeyFile;

    if (devConfig.server.authType == Core::SshConnectionParameters::AuthByPwd)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->detailsWidget->setEnabled(true);
    m_nameValidator->setDisplayName(devConfig.name);
    m_ui->timeoutSpinBox->setValue(devConfig.server.timeout);
    fillInValues();
}

void MaemoSettingsWidget::fillInValues()
{
    m_ui->nameLineEdit->setText(currentConfig().name);
    m_ui->hostLineEdit->setText(currentConfig().server.host);
    m_ui->sshPortSpinBox->setValue(currentConfig().server.port);
    m_ui->portsLineEdit->setText(currentConfig().portsSpec);
    m_ui->timeoutSpinBox->setValue(currentConfig().server.timeout);
    m_ui->userLineEdit->setText(currentConfig().server.uname);
    m_ui->pwdLineEdit->setText(currentConfig().server.pwd);
    m_ui->keyFileLineEdit->setPath(currentConfig().server.privateKeyFile);
    m_ui->showPasswordCheckBox->setChecked(false);
    updatePortsWarningLabel();
    const bool isSimulator
        = currentConfig().type == MaemoDeviceConfig::Simulator;
    m_ui->hostLineEdit->setReadOnly(isSimulator);
    m_ui->sshPortSpinBox->setReadOnly(isSimulator);
}

void MaemoSettingsWidget::saveSettings()
{
    // We must defer this step because of a stupid bug on MacOS. See QTCREATORBUG-1675.
    m_saveSettingsRequested = true;
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
    const QString name = currentConfig().name;
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
    currentConfig().name = name;
    fillInValues();
}

void MaemoSettingsWidget::authenticationTypeChanged()
{
    const bool usePassword = m_ui->passwordButton->isChecked();
    currentConfig().server.authType
        = usePassword ? Core::SshConnectionParameters::AuthByPwd : Core::SshConnectionParameters::AuthByKey;
    m_ui->pwdLineEdit->setEnabled(usePassword);
    m_ui->passwordLabel->setEnabled(usePassword);
    m_ui->keyFileLineEdit->setEnabled(!usePassword);
    m_ui->keyLabel->setEnabled(!usePassword);
}

void MaemoSettingsWidget::hostNameEditingFinished()
{
    currentConfig().server.host = m_ui->hostLineEdit->text();
}

void MaemoSettingsWidget::sshPortEditingFinished()
{
    currentConfig().server.port = m_ui->sshPortSpinBox->value();
}

void MaemoSettingsWidget::handleFreePortsChanged()
{
    currentConfig().portsSpec = m_ui->portsLineEdit->text();
    updatePortsWarningLabel();
}

void MaemoSettingsWidget::timeoutEditingFinished()
{
    currentConfig().server.timeout = m_ui->timeoutSpinBox->value();
}

void MaemoSettingsWidget::userNameEditingFinished()
{
    currentConfig().server.uname = m_ui->userLineEdit->text();
}

void MaemoSettingsWidget::passwordEditingFinished()
{
    currentConfig().server.pwd = m_ui->pwdLineEdit->text();
}

void MaemoSettingsWidget::keyFileEditingFinished()
{
    currentConfig().server.privateKeyFile = m_ui->keyFileLineEdit->path();
}

void MaemoSettingsWidget::showPassword(bool showClearText)
{
    m_ui->pwdLineEdit->setEchoMode(showClearText
        ? QLineEdit::Normal : QLineEdit::Password);
}

void MaemoSettingsWidget::testConfig()
{
    QDialog *dialog = new MaemoConfigTestDialog(currentConfig(), this);
    dialog->open();
}

void MaemoSettingsWidget::showGenerateSshKeyDialog()
{
    MaemoSshConfigDialog dialog(this);
    dialog.exec();
}

void MaemoSettingsWidget::setPrivateKey(const QString &path)
{
    m_ui->keyFileLineEdit->setPath(path);
    keyFileEditingFinished();
}

void MaemoSettingsWidget::deployKey()
{
    if (m_keyDeployer)
        return;

    disconnect(m_ui->deployKeyButton, 0, this, 0);
    m_ui->deployKeyButton->setText(tr("Stop Deploying"));
    connect(m_ui->deployKeyButton, SIGNAL(clicked()), this,
        SLOT(stopDeploying()));
    m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionFailure()));
    m_connection->connectToHost(currentConfig().server);
}

void MaemoSettingsWidget::handleConnected()
{
    if (!m_connection)
        return;

    const QString &dir
        = QFileInfo(currentConfig().server.privateKeyFile).path();
    QString publicKeyFileName = QFileDialog::getOpenFileName(this,
        tr("Choose Public Key File"), dir,
        tr("Public Key Files(*.pub);;All Files (*)"));
    if (publicKeyFileName.isEmpty()) {
        stopDeploying();
        return;
    }

    QFile keyFile(publicKeyFileName);
    QByteArray key;
    const bool keyFileAccessible = keyFile.open(QIODevice::ReadOnly);
    if (keyFileAccessible)
        key = keyFile.readAll();
    if (!keyFileAccessible || keyFile.error() != QFile::NoError) {
        QMessageBox::critical(this, tr("Deployment Failed"),
            tr("Could not read public key file '%1'.").arg(publicKeyFileName));
        stopDeploying();
        return;
    }

    const QByteArray command = "test -d .ssh "
        "|| mkdir .ssh && chmod 0700 .ssh && echo '"
        + key + "' >> .ssh/authorized_keys";
    m_keyDeployer = m_connection->createRemoteProcess(command);
    connect(m_keyDeployer.data(), SIGNAL(closed(int)), this,
        SLOT(handleKeyUploadFinished(int)));
    m_keyDeployer->start();
}

void MaemoSettingsWidget::handleConnectionFailure()
{
    if (!m_connection)
        return;

    QMessageBox::critical(this, tr("Deployment Failed"),
        tr("Could not connect to host: %1").arg(m_connection->errorString()));
    stopDeploying();
}

void MaemoSettingsWidget::handleKeyUploadFinished(int exitStatus)
{
    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (!m_connection)
        return;

    if (exitStatus == SshRemoteProcess::ExitedNormally
        && m_keyDeployer->exitCode() == 0) {
        QMessageBox::information(this, tr("Deployment Succeeded"),
            tr("Key was successfully deployed."));
    } else {
        QMessageBox::critical(this, tr("Deployment Failed"),
            tr("Key deployment failed: %1.").arg(m_keyDeployer->errorString()));
    }
    stopDeploying();
}

void MaemoSettingsWidget::stopDeploying()
{
    if (m_keyDeployer) {
        disconnect(m_keyDeployer.data(), 0, this, 0);
        m_keyDeployer = SshRemoteProcess::Ptr();
    }
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);
    m_ui->deployKeyButton->disconnect();
    m_ui->deployKeyButton->setText(tr("Deploy Public Key ..."));
    connect(m_ui->deployKeyButton, SIGNAL(clicked()), this, SLOT(deployKey()));
}

void MaemoSettingsWidget::currentConfigChanged(int index)
{
    stopDeploying();
    if (index == -1) {
        m_ui->removeConfigButton->setEnabled(false);
        m_ui->testConfigButton->setEnabled(false);
        m_ui->generateKeyButton->setEnabled(false);
        m_ui->deployKeyButton->setEnabled(false);
        clearDetails();
        m_ui->detailsWidget->setEnabled(false);
    } else {
        m_ui->removeConfigButton->setEnabled(true);
        m_ui->testConfigButton->setEnabled(true);
        m_ui->generateKeyButton->setEnabled(true);
        m_ui->deployKeyButton->setEnabled(true);
        m_ui->configurationComboBox->setCurrentIndex(index);
        display(currentConfig());
    }
}

void MaemoSettingsWidget::clearDetails()
{
    m_ui->hostLineEdit->clear();
    m_ui->sshPortSpinBox->clear();
    m_ui->timeoutSpinBox->clear();
    m_ui->userLineEdit->clear();
    m_ui->pwdLineEdit->clear();
    m_ui->portsLineEdit->clear();
    m_ui->portsWarningLabel->clear();
}

void MaemoSettingsWidget::updatePortsWarningLabel()
{
    if (currentConfig().freePorts().hasMore()) {
        m_ui->portsWarningLabel->clear();
    } else {
        m_ui->portsWarningLabel->setText(QLatin1String("<font color=\"red\">")
            + tr("You will need at least one port.") + QLatin1String("</font>"));
    }
}

} // namespace Internal
} // namespace Qt4ProjectManager
