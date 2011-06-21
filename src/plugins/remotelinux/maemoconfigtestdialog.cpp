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

#include "maemoconfigtestdialog.h"
#include "ui_maemoconfigtestdialog.h"

#include "linuxdeviceconfiguration.h"
#include "maemoglobal.h"
#include "maemousedportsgatherer.h"

#include <utils/ssh/sshremoteprocessrunner.h>

#include <QtGui/QPalette>
#include <QtGui/QPushButton>

using namespace Utils;

namespace RemoteLinux {
namespace Internal {
namespace {
const char QmlToolingDirectory[] = "/usr/lib/qt4/plugins/qmltooling";
} // anonymous namespace

MaemoConfigTestDialog::MaemoConfigTestDialog(const LinuxDeviceConfiguration::ConstPtr &config,
        QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui_MaemoConfigTestDialog)
    , m_config(config)
    , m_portsGatherer(new MaemoUsedPortsGatherer(this))
{
    m_ui->setupUi(this);
    m_closeButton = m_ui->buttonBox->button(QDialogButtonBox::Close);

    connect(m_closeButton, SIGNAL(clicked()), SLOT(stopConfigTest()));
    connect(m_portsGatherer, SIGNAL(error(QString)),
        SLOT(handlePortListFailure(QString)));
    connect(m_portsGatherer, SIGNAL(portListReady()),
        SLOT(handlePortListReady()));

    startConfigTest();
}

MaemoConfigTestDialog::~MaemoConfigTestDialog()
{
    stopConfigTest();
}

void MaemoConfigTestDialog::startConfigTest()
{
    if (m_testProcessRunner)
        return;

    m_currentTest = GeneralTest;
    const QString testingText = m_config->type() == LinuxDeviceConfiguration::Emulator
        ? tr("Testing configuration. This may take a while.")
        : tr("Testing configuration...");
    m_ui->testResultEdit->setPlainText(testingText);
    m_closeButton->setText(tr("Stop Test"));

    // We need to explicitly create the connection here, because the other
    // constructor uses a managed connection, i.e. it might re-use an
    // existing one, which we explicitly don't want here.
    m_testProcessRunner = SshRemoteProcessRunner::create(SshConnection::create(m_config->sshParameters()));

    connect(m_testProcessRunner.data(), SIGNAL(connectionError(Utils::SshError)),
        this, SLOT(handleConnectionError()));
    connect(m_testProcessRunner.data(), SIGNAL(processClosed(int)), this,
        SLOT(handleTestProcessFinished(int)));
    connect(m_testProcessRunner.data(),
        SIGNAL(processOutputAvailable(QByteArray)), this,
        SLOT(processSshOutput(QByteArray)));
    const QLatin1String sysInfoCmd("uname -rsm");
    QString command = sysInfoCmd;
    QString qtInfoCmd;
    switch (MaemoGlobal::packagingSystem(m_config->osType())) {
    case MaemoGlobal::Rpm:
        qtInfoCmd = QLatin1String("rpm -qa 'libqt*' "
            "--queryformat '%{NAME} %{VERSION}\\n'");
        break;
    case MaemoGlobal::Dpkg:
        qtInfoCmd = QLatin1String("dpkg-query -W -f "
            "'${Package} ${Version} ${Status}\n' 'libqt*' |grep ' installed$'");
        break;
    default:
        break;
    }
    if (!qtInfoCmd.isEmpty())
        command += QLatin1String(" && ") + qtInfoCmd;
    m_testProcessRunner->run(command.toUtf8());
}

void MaemoConfigTestDialog::handleConnectionError()
{
    if (!m_testProcessRunner)
        return;
    QString output = tr("Could not connect to host: %1")
        .arg(m_testProcessRunner->connection()->errorString());
    if (m_config->type() == LinuxDeviceConfiguration::Emulator)
        output += tr("\nDid you start Qemu?");
    m_ui->testResultEdit->setPlainText(output);
    stopConfigTest();
}

void MaemoConfigTestDialog::handleTestProcessFinished(int exitStatus)
{
    if (!m_testProcessRunner)
        return;

    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    switch (m_currentTest) {
    case GeneralTest:
        handleGeneralTestResult(exitStatus);
        break;
    case MadDeveloperTest:
        handleMadDeveloperTestResult(exitStatus);
        break;
    case QmlToolingTest:
        handleQmlToolingTestResult(exitStatus);
        break;
    default:
        qDebug("%s: Unexpected test state %d.", Q_FUNC_INFO, m_currentTest);
    }
}

void MaemoConfigTestDialog::handleGeneralTestResult(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_testProcessRunner->process()->exitCode() != 0) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_testProcessRunner->process()->errorString()));
   } else {
        const QString &output = parseTestOutput();
        if (!m_qtVersionOk) {
            m_ui->errorLabel->setText(tr("Qt version mismatch! "
                " Expected Qt on device: 4.6.2 or later."));
        }
        m_ui->testResultEdit->setPlainText(output);
    }

    if (m_config->osType() == LinuxDeviceConfiguration::Maemo5OsType
            || m_config->osType() == LinuxDeviceConfiguration::HarmattanOsType
            || m_config->osType() == LinuxDeviceConfiguration::MeeGoOsType) {
        m_currentTest = MadDeveloperTest;
        disconnect(m_testProcessRunner.data(),
            SIGNAL(processOutputAvailable(QByteArray)), this,
            SLOT(processSshOutput(QByteArray)));
        m_testProcessRunner->run("test -x "
            + MaemoGlobal::devrootshPath().toUtf8());
    } else {
        testPorts();
    }
}

void MaemoConfigTestDialog::handleMadDeveloperTestResult(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_testProcessRunner->process()->errorString()));
    } else if (m_testProcessRunner->process()->exitCode() != 0) {
        QString errorMsg = m_ui->errorLabel->text() + QLatin1String("<br>")
            + tr("%1 is not installed.<br>You will not be able to deploy "
                 "to this device.")
                .arg(MaemoGlobal::madDeveloperUiName(m_config->osType()));
        if (m_config->osType() == LinuxDeviceConfiguration::HarmattanOsType) {
            errorMsg += QLatin1String("<br>")
                + tr("Please switch the device to developer mode via Settings -> Security.");
        }
        m_ui->errorLabel->setText(errorMsg);
    }

    if (m_config->osType() == LinuxDeviceConfiguration::HarmattanOsType) {
        m_currentTest = QmlToolingTest;
        m_testProcessRunner->run(QByteArray("test -d ") + QmlToolingDirectory);
    } else {
        testPorts();
    }
}

void MaemoConfigTestDialog::handleQmlToolingTestResult(int exitStatus)
{
    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_testProcessRunner->process()->errorString()));
    } else if (m_testProcessRunner->process()->exitCode() != 0) {
        QString errorMsg = m_ui->errorLabel->text() + QLatin1String("<br>")
            + tr("Missing directory '%1'. You will not be able to do QML debugging on this device.")
                .arg(QmlToolingDirectory);
        m_ui->errorLabel->setText(errorMsg);
    }

    testPorts();
}

void MaemoConfigTestDialog::handlePortListFailure(const QString &errMsg)
{
    m_ui->testResultEdit->appendPlainText(tr("Error retrieving list of used ports: %1")
        .arg(errMsg));
    finish();
}

void MaemoConfigTestDialog::handlePortListReady()
{
    const QList<int> &usedPorts = m_portsGatherer->usedPorts();
    QString output;
    if (usedPorts.isEmpty()) {
        output = tr("All specified ports are available.");
    } else {
        output = tr("The following supposedly free ports are being used on the device:");
        foreach (const int port, usedPorts)
            output += QLatin1Char(' ') + QString::number(port);
    }
    m_ui->testResultEdit->appendPlainText(output);
    finish();
}

void MaemoConfigTestDialog::testPorts()
{
    if (m_config->freePorts().hasMore())
        m_portsGatherer->start(m_testProcessRunner->connection(), m_config);
    else
        finish();
}

void MaemoConfigTestDialog::finish()
{
    if (m_ui->errorLabel->text().isEmpty()) {
        QPalette palette = m_ui->errorLabel->palette();
        palette.setColor(m_ui->errorLabel->foregroundRole(),
            QColor(QLatin1String("blue")));
        m_ui->errorLabel->setPalette(palette);
        m_ui->errorLabel->setText(tr("Device configuration okay."));
    }
    stopConfigTest();
}

void MaemoConfigTestDialog::stopConfigTest()
{
    if (m_testProcessRunner) {
        disconnect(m_testProcessRunner.data(), 0, this, 0);
        m_testProcessRunner = SshRemoteProcessRunner::Ptr();
    }
    m_deviceTestOutput.clear();
    m_closeButton->setText(tr("Close"));
}

void MaemoConfigTestDialog::processSshOutput(const QByteArray &output)
{
    m_deviceTestOutput.append(QString::fromUtf8(output));
}

QString MaemoConfigTestDialog::parseTestOutput()
{
    m_qtVersionOk = false;

    QString output;
    const QRegExp unamePattern(QLatin1String("Linux (\\S+)\\s(\\S+)"));
    int index = unamePattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output = tr("Device configuration test failed: Unexpected output:\n%1")
            .arg(m_deviceTestOutput);
        return output;
    }

    output = tr("Hardware architecture: %1\n").arg(unamePattern.cap(2));
    output.append(tr("Kernel version: %1\n").arg(unamePattern.cap(1)));

    QString patternString;
    switch (MaemoGlobal::packagingSystem(m_config->osType())) {
    case MaemoGlobal::Rpm:
        patternString = QLatin1String("(libqt\\S+) ((\\d+)\\.(\\d+)\\.(\\d+))");
        break;
    case MaemoGlobal::Dpkg:
        patternString = QLatin1String("(\\S+) (\\S*(\\d+)\\.(\\d+)\\.(\\d+)\\S*) \\S+ \\S+ \\S+");
        break;
    default:
        m_qtVersionOk = true;
        return output;
    }

    const QRegExp packagePattern(patternString);
    index = packagePattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output.append(tr("No Qt packages installed."));
        return output;
    }

    output.append(tr("List of installed Qt packages:") + QLatin1Char('\n'));
    do {
        output.append(QLatin1Char('\t') + packagePattern.cap(1) + QLatin1Char(' ')
            + packagePattern.cap(2) + QLatin1Char('\n'));
        index = packagePattern.indexIn(m_deviceTestOutput, index
            + packagePattern.cap(0).length());
        if (!m_qtVersionOk && QT_VERSION_CHECK(packagePattern.cap(3).toInt(),
            packagePattern.cap(4).toInt(), packagePattern.cap(5).toInt()) >= 0x040602) {
            m_qtVersionOk = true;
        }
    } while (index != -1);
    return output;
}

} // namespace Internal
} // namespace RemoteLinux
