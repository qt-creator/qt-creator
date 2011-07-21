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
#include "linuxdevicetestdialog.h"
#include "ui_linuxdevicetestdialog.h"

#include <QtGui/QPushButton>

namespace RemoteLinux {

LinuxDeviceTestDialog::LinuxDeviceTestDialog(const QSharedPointer<const LinuxDeviceConfiguration> &deviceConfiguration,
        AbstractLinuxDeviceTester *deviceTester, QWidget *parent)
    : QDialog(parent),
      m_ui(new Ui::LinuxDeviceTestDialog),
      m_deviceTester(deviceTester),
      m_finished(false)
{
    m_ui->setupUi(this);

    connect(m_deviceTester, SIGNAL(progressMessage(QString)), SLOT(handleProgressMessage(QString)));
    connect(m_deviceTester, SIGNAL(errorMessage(QString)), SLOT(handleErrorMessage(QString)));
    connect(m_deviceTester, SIGNAL(finished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)),
        SLOT(handleTestFinished(RemoteLinux::AbstractLinuxDeviceTester::TestResult)));
    m_deviceTester->testDevice(deviceConfiguration);
}

LinuxDeviceTestDialog::~LinuxDeviceTestDialog()
{
    delete m_ui;
}

void LinuxDeviceTestDialog::reject()
{
    if (!m_finished)
        m_deviceTester->stopTest();
    QDialog::reject();
}

void LinuxDeviceTestDialog::handleProgressMessage(const QString &message)
{
    m_ui->textEdit->appendPlainText(message);
}

void LinuxDeviceTestDialog::handleErrorMessage(const QString &message)
{
    m_ui->textEdit->appendHtml(QLatin1String("<font color=\"red\">") + message
        + QLatin1String("</font><p/><p/>"));
}

void LinuxDeviceTestDialog::handleTestFinished(AbstractLinuxDeviceTester::TestResult result)
{
    m_finished = true;
    m_ui->buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    if (result == AbstractLinuxDeviceTester::TestSuccess) {
        m_ui->textEdit->appendHtml(QLatin1String("<b><font color=\"blue\">")
            + tr("Device test finished successfully.") + QLatin1String("</font></b>"));
    } else {
        m_ui->textEdit->appendHtml(QLatin1String("<b><font color=\"red\">")
            + tr("Device test failed.") + QLatin1String("</font></b><p/>"));
    }
}

} // namespace RemoteLinux
