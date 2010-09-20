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

#include "maemoconfigtestdialog.h"
#include "ui_maemoconfigtestdialog.h"

#include "maemodeviceconfigurations.h"
#include "maemoglobal.h"

#include <coreplugin/ssh/sshconnection.h>
#include <coreplugin/ssh/sshremoteprocess.h>

#include <QtGui/QPalette>
#include <QtGui/QPushButton>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

MaemoConfigTestDialog::MaemoConfigTestDialog(const MaemoDeviceConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui_MaemoConfigTestDialog)
    , m_config(config)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);
    m_closeButton = m_ui->buttonBox->button(QDialogButtonBox::Close);

    connect(m_closeButton, SIGNAL(clicked()), SLOT(stopConfigTest()));

    startConfigTest();
}

MaemoConfigTestDialog::~MaemoConfigTestDialog()
{
    stopConfigTest();
}

void MaemoConfigTestDialog::startConfigTest()
{
    if (m_infoProcess)
        return;

    m_ui->testResultEdit->setPlainText(tr("Testing configuration..."));
    m_closeButton->setText(tr("Stop Test"));
    m_connection = SshConnection::create();
    connect(m_connection.data(), SIGNAL(connected()), this,
        SLOT(handleConnected()));
    connect(m_connection.data(), SIGNAL(error(SshError)), this,
        SLOT(handleConnectionError()));
    m_connection->connectToHost(m_config.server);
}

void MaemoConfigTestDialog::handleConnected()
{
    if (!m_connection)
        return;
    QLatin1String sysInfoCmd("uname -rsm");
    QLatin1String qtInfoCmd("dpkg -l |grep libqt |grep '^ii'"
        "|sed 's/[[:space:]][[:space:]]*/ /g' "
        "|cut -d ' ' -f 2,3 |sed 's/~.*//g'");
    QString command(sysInfoCmd + " && " + qtInfoCmd);
    m_infoProcess = m_connection->createRemoteProcess(command.toUtf8());
    connect(m_infoProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleInfoProcessFinished(int)));
    connect(m_infoProcess.data(), SIGNAL(outputAvailable(QByteArray)), this,
        SLOT(processSshOutput(QByteArray)));
    m_infoProcess->start();
}

void MaemoConfigTestDialog::handleConnectionError()
{
    if (!m_connection)
        return;
    QString output = tr("Could not connect to host: %1")
        .arg(m_connection->errorString());
    if (m_config.type == MaemoDeviceConfig::Simulator)
        output += tr("\nDid you start Qemu?");
    m_ui->testResultEdit->setPlainText(output);
    stopConfigTest();
}

void MaemoConfigTestDialog::handleInfoProcessFinished(int exitStatus)
{
    if (!m_connection)
        return;

    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (!m_infoProcess)
        return;

    if (exitStatus != SshRemoteProcess::ExitedNormally
        || m_infoProcess->exitCode() != 0) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_infoProcess->errorString()));
    } else {
        const QString &output = parseTestOutput();
        if (!m_qtVersionOk) {
            m_ui->errorLabel->setText(tr("Qt version mismatch! "
                " Expected Qt on device: 4.6.2 or later."));
        }
        m_ui->testResultEdit->setPlainText(output);
    }

    const QByteArray command = "test -x " + MaemoGlobal::remoteSudo().toUtf8();
    m_madDeveloperTestProcess = m_connection->createRemoteProcess(command);
    connect(m_madDeveloperTestProcess.data(), SIGNAL(closed(int)), this,
        SLOT(handleMadDeveloperTestProcessFinished(int)));
    m_madDeveloperTestProcess->start();
}

void MaemoConfigTestDialog::handleMadDeveloperTestProcessFinished(int exitStatus)
{
    if (!m_connection)
        return;

    Q_ASSERT(exitStatus == SshRemoteProcess::FailedToStart
        || exitStatus == SshRemoteProcess::KilledBySignal
        || exitStatus == SshRemoteProcess::ExitedNormally);

    if (exitStatus != SshRemoteProcess::ExitedNormally) {
        m_ui->testResultEdit->setPlainText(tr("Remote process failed: %1")
            .arg(m_madDeveloperTestProcess->errorString()));
    } else if (m_madDeveloperTestProcess->exitCode() != 0) {
        m_ui->errorLabel->setText(m_ui->errorLabel->text()
            + QLatin1String("<br>") + tr("Mad Developer is not installed.<br>"
                  "You will not be able to deploy to this device."));
    }
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
    if (m_infoProcess)
        disconnect(m_infoProcess.data(), 0, this, 0);
    if (m_madDeveloperTestProcess)
        disconnect(m_madDeveloperTestProcess.data(), 0, this, 0);
    if (m_connection)
        disconnect(m_connection.data(), 0, this, 0);

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
    const QRegExp dkpgPattern(QLatin1String("libqt\\S+ (\\d)\\.(\\d)\\.(\\d)"));
    index = dkpgPattern.indexIn(m_deviceTestOutput);
    if (index == -1) {
        output.append(tr("No Qt packages installed."));
        return output;
    }

    output.append(tr("List of installed Qt packages:") + QLatin1Char('\n'));
    do {
        output.append(QLatin1Char('\t') + dkpgPattern.cap(0)
            + QLatin1Char('\n'));
        index = dkpgPattern.indexIn(m_deviceTestOutput, index + 1);
        if (!m_qtVersionOk && QT_VERSION_CHECK(dkpgPattern.cap(1).toInt(),
            dkpgPattern.cap(2).toInt(), dkpgPattern.cap(3).toInt()) >= 0x040602) {
            m_qtVersionOk = true;
        }
    } while (index != -1);
    return output;
}

} // namespace Internal
} // namespace Qt4ProjectManager
