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

#include "maemodeviceconfigurations.h"
#include "maemosshthread.h"

#include <QtCore/QRegExp>
#include <QtGui/QFileDialog>
#include <QtCore/QFileInfo>
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

class PortAndTimeoutValidator : public QIntValidator
{
public:
    PortAndTimeoutValidator() : QIntValidator(0, SHRT_MAX, 0)
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
      m_ui(new Ui_maemoSettingsWidget),
      m_devConfs(MaemoDeviceConfigurations::instance().devConfigs()),
      m_nameValidator(new NameValidator(m_devConfs)),
      m_sshPortValidator(new PortAndTimeoutValidator),
      m_gdbServerPortValidator(new PortAndTimeoutValidator),
      m_timeoutValidator(new PortAndTimeoutValidator),
      m_deviceTester(0),
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
    m_ui->sshPortLineEdit->setValidator(m_sshPortValidator);
    m_ui->gdbServerPortLineEdit->setValidator(m_gdbServerPortValidator);
    m_ui->timeoutLineEdit->setValidator(m_timeoutValidator);
    m_ui->keyFileLineEdit->setExpectedKind(Utils::PathChooser::File);
    foreach(const MaemoDeviceConfig &devConf, m_devConfs)
        m_ui->configListWidget->addItem(devConf.name);
    m_defaultTestOutput = m_ui->testResultEdit->toPlainText();
    if (m_devConfs.count() == 1)
        m_ui->configListWidget->setCurrentRow(0, QItemSelectionModel::Select);
}

void MaemoSettingsWidget::addConfig()
{
    QLatin1String prefix("New Device Configuration ");
    int suffix = 1;
    QString newName;
    bool isUnique = false;
    do {
        newName = prefix + QString::number(suffix++);
        isUnique = !configNameExists(m_devConfs, newName);
    } while (!isUnique);

    m_devConfs.append(MaemoDeviceConfig(newName));
    m_ui->configListWidget->addItem(newName);
    m_ui->configListWidget->setCurrentRow(m_ui->configListWidget->count() - 1);
    m_ui->nameLineEdit->selectAll();
    m_ui->removeConfigButton->setEnabled(true);
    m_ui->nameLineEdit->setFocus();
}

void MaemoSettingsWidget::deleteConfig()
{
    const QList<QListWidgetItem *> &selectedItems =
        m_ui->configListWidget->selectedItems();
    if (selectedItems.isEmpty())
        return;
    QListWidgetItem *item = selectedItems.first();
    const int selectedRow = m_ui->configListWidget->row(item);
    m_devConfs.removeAt(selectedRow);
    disconnect(m_ui->configListWidget, SIGNAL(itemSelectionChanged()), 0, 0);
    delete m_ui->configListWidget->takeItem(selectedRow);
    connect(m_ui->configListWidget, SIGNAL(itemSelectionChanged()),
            this, SLOT(selectionChanged()));
    Q_ASSERT(m_ui->configListWidget->count() == m_devConfs.count());
    selectionChanged();
}

void MaemoSettingsWidget::display(const MaemoDeviceConfig &devConfig)
{
    m_ui->nameLineEdit->setText(devConfig.name);
    if (devConfig.type == MaemoDeviceConfig::Physical)
        m_ui->deviceButton->setChecked(true);
    else
        m_ui->simulatorButton->setChecked(true);

    if (devConfig.authentication == MaemoDeviceConfig::Password)
        m_ui->passwordButton->setChecked(true);
    else
        m_ui->keyButton->setChecked(true);
    m_ui->detailsWidget->setEnabled(true);
    m_nameValidator->setDisplayName(devConfig.name);
    m_sshPortValidator->setValue(devConfig.sshPort);
    m_gdbServerPortValidator->setValue(devConfig.gdbServerPort);
    m_timeoutValidator->setValue(devConfig.timeout);
}

void MaemoSettingsWidget::saveSettings()
{
    MaemoDeviceConfigurations::instance().setDevConfigs(m_devConfs);
}

MaemoDeviceConfig &MaemoSettingsWidget::currentConfig()
{
    Q_ASSERT(m_ui->configListWidget->count() == m_devConfs.count());
    const QList<QListWidgetItem *> &selectedItems =
        m_ui->configListWidget->selectedItems();
    Q_ASSERT(selectedItems.count() == 1);
    const int selectedRow = m_ui->configListWidget->row(selectedItems.first());
    Q_ASSERT(selectedRow < m_devConfs.count());
    return m_devConfs[selectedRow];
}

void MaemoSettingsWidget::configNameEditingFinished()
{
    const QString &newName = m_ui->nameLineEdit->text();
    currentConfig().name = newName;
    m_nameValidator->setDisplayName(newName);
    m_ui->configListWidget->currentItem()->setText(newName);
}

void MaemoSettingsWidget::deviceTypeChanged()
{
    currentConfig().type =
        m_ui->deviceButton->isChecked()
            ? MaemoDeviceConfig::Physical
            : MaemoDeviceConfig::Simulator;

    // Port values for the simulator are specified by Qemu's
    // "information" file, to which we have no access here,
    // so we hard-code the last known values.
    if (currentConfig().type == MaemoDeviceConfig::Simulator) {
        currentConfig().host = QLatin1String("localhost");
        currentConfig().sshPort = 6666;
        currentConfig().gdbServerPort = 13219;
        m_ui->hostLineEdit->setReadOnly(true);
        m_ui->sshPortLineEdit->setReadOnly(true);
        m_ui->gdbServerPortLineEdit->setReadOnly(true);
    } else {
        m_ui->hostLineEdit->setReadOnly(false);
        m_ui->sshPortLineEdit->setReadOnly(false);
        m_ui->gdbServerPortLineEdit->setReadOnly(false);
    }
    m_ui->hostLineEdit->setText(currentConfig().host);
    m_ui->sshPortLineEdit->setText(QString::number(currentConfig().sshPort));
    m_ui->gdbServerPortLineEdit
        ->setText(QString::number(currentConfig().gdbServerPort));
    m_ui->timeoutLineEdit->setText(QString::number(currentConfig().timeout));
    m_ui->userLineEdit->setText(currentConfig().uname);
    m_ui->pwdLineEdit->setText(currentConfig().pwd);
    m_ui->keyFileLineEdit->setPath(currentConfig().keyFile);
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
    setPortOrTimeout(m_ui->sshPortLineEdit, currentConfig().sshPort,
                     m_sshPortValidator);
}

void MaemoSettingsWidget::gdbServerPortEditingFinished()
{
    setPortOrTimeout(m_ui->gdbServerPortLineEdit, currentConfig().gdbServerPort,
                     m_gdbServerPortValidator);
}

void MaemoSettingsWidget::timeoutEditingFinished()
{
    setPortOrTimeout(m_ui->timeoutLineEdit, currentConfig().timeout,
                     m_timeoutValidator);
}

void MaemoSettingsWidget::setPortOrTimeout(const QLineEdit *lineEdit,
    int &confVal, PortAndTimeoutValidator *validator)
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
    if (m_deviceTester)
        return;

    m_ui->testConfigButton->disconnect();
    m_ui->testResultEdit->setPlainText(m_defaultTestOutput);
    QLatin1String sysInfoCmd("uname -rsm");
    QLatin1String qtInfoCmd("dpkg -l |grep libqt "
        "|sed 's/[[:space:]][[:space:]]*/ /g' "
        "|cut -d ' ' -f 2,3 |sed 's/~.*//g'");
    QString command(sysInfoCmd + " && " + qtInfoCmd);
    m_deviceTester = new MaemoSshRunner(currentConfig(), command);
    connect(m_deviceTester, SIGNAL(remoteOutput(QString)),
            this, SLOT(processSshOutput(QString)));
    connect(m_deviceTester, SIGNAL(finished()),
            this, SLOT(handleTestThreadFinished()));
    m_ui->testConfigButton->setText(tr("Stop test"));
    connect(m_ui->testConfigButton, SIGNAL(clicked()),
            this, SLOT(stopConfigTest()));
    m_deviceTester->start();
}

void MaemoSettingsWidget::processSshOutput(const QString &data)
{
    qDebug("%s", qPrintable(data));
    m_deviceTestOutput.append(data);
}

void MaemoSettingsWidget::handleTestThreadFinished()
{
    if (!m_deviceTester)
        return;

    QString output;
    if (m_deviceTester->hasError()) {
        output = tr("Device configuration test failed:\n%1").arg(m_deviceTester->error());
        if (currentConfig().type == MaemoDeviceConfig::Simulator)
            output.append(tr("\nDid you start Qemu?"));
    } else {
        output = parseTestOutput();
    }
    m_ui->testResultEdit->setPlainText(output);
    stopConfigTest();
}

void MaemoSettingsWidget::stopConfigTest()
{
    if (m_deviceTester) {
        m_ui->testConfigButton->disconnect();
        const bool buttonWasEnabled = m_ui->testConfigButton->isEnabled();
        m_deviceTester->disconnect();
        m_deviceTester->stop();
        delete m_deviceTester;
        m_deviceTester = 0;
        m_deviceTestOutput.clear();
        m_ui->testConfigButton->setText(tr("Test"));
        connect(m_ui->testConfigButton, SIGNAL(clicked()),
                this, SLOT(testConfig()));
        m_ui->testConfigButton->setEnabled(buttonWasEnabled);
    }
}

QString MaemoSettingsWidget::parseTestOutput()
{
    QString output;
    const QRegExp unamePattern(QLatin1String("Linux (\\S+)\\s(\\S+)"));
    int index = unamePattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output = tr("Device configuration test failed: Unexpected output:\n%1").arg(m_deviceTestOutput);
        return output;
    }

    output = tr("Hardware architecture: %1\n").arg(unamePattern.cap(2));
    output.append(tr("Kernel version: %1\n").arg(unamePattern.cap(1)));
    output.prepend(tr("Device configuration successful.\n"));
    const QRegExp dkpgPattern(QLatin1String("libqt\\S+ \\d\\.\\d\\.\\d"));
    index = dkpgPattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output.append("No Qt packages installed.");
        return output;
    }
    output.append("List of installed Qt packages:\n");
    do {
        output.append(QLatin1String("\t") + dkpgPattern.cap(0)
                      + QLatin1String("\n"));
        index = dkpgPattern.indexIn(m_deviceTestOutput, index + 1);
    } while (index != -1);
    return output;
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

    QString output;
    if (m_keyDeployer->hasError()) {
        output = tr("Key deployment failed: %1").arg(m_keyDeployer->error());
    } else {
        output = tr("Key was successfully deployed.");
    }
    m_ui->testResultEdit->setPlainText(output);
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

void MaemoSettingsWidget::selectionChanged()
{
    const QList<QListWidgetItem *> &selectedItems =
        m_ui->configListWidget->selectedItems();
    Q_ASSERT(selectedItems.count() <= 1);
    stopConfigTest();
    stopDeploying();
    m_ui->testResultEdit->setPlainText(m_defaultTestOutput);
    if (selectedItems.isEmpty()) {
        m_ui->removeConfigButton->setEnabled(false);
        m_ui->testConfigButton->setEnabled(false);
        clearDetails();
        m_ui->detailsWidget->setEnabled(false);
    } else {
        m_ui->removeConfigButton->setEnabled(true);
        m_ui->testConfigButton->setEnabled(true);
        display(currentConfig());
    }
}

void MaemoSettingsWidget::clearDetails()
{
    m_ui->nameLineEdit->clear();
    m_ui->hostLineEdit->clear();
    m_ui->sshPortLineEdit->clear();
    m_ui->gdbServerPortLineEdit->clear();
    m_ui->timeoutLineEdit->clear();
    m_ui->userLineEdit->clear();
    m_ui->pwdLineEdit->clear();
}

} // namespace Internal
} // namespace Qt4ProjectManager
