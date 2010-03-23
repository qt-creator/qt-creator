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

#include "maemosshthread.h"

#include <QtGui/QPushButton>

namespace {

/**
 * Class that waits until a thread is finished and then deletes it, and then
 * schedules itself to be deleted.
 */
class SafeThreadDeleter : public QThread
{
public:
    SafeThreadDeleter(QThread *thread) : m_thread(thread) {}
    ~SafeThreadDeleter() { wait(); }

protected:
    void run()
    {
        // Wait for m_thread to finish and then delete it
        m_thread->wait();
        delete m_thread;

        // Schedule this thread for deletion
        deleteLater();
    }

private:
    QThread *m_thread;
};

} // anonymous namespace

namespace Qt4ProjectManager {
namespace Internal {

MaemoConfigTestDialog::MaemoConfigTestDialog(const MaemoDeviceConfig &config, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui_MaemoConfigTestDialog)
    , m_config(config)
    , m_deviceTester(0)
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
    if (m_deviceTester)
        return;

    m_ui->testResultEdit->setPlainText(tr("Testing configuration..."));
    m_closeButton->setText(tr("Stop Test"));

    QLatin1String sysInfoCmd("uname -rsm");
    QLatin1String qtInfoCmd("dpkg -l |grep libqt "
        "|sed 's/[[:space:]][[:space:]]*/ /g' "
        "|cut -d ' ' -f 2,3 |sed 's/~.*//g'");
    QString command(sysInfoCmd + " && " + qtInfoCmd);
    m_deviceTester = new MaemoSshRunner(m_config, command);
    connect(m_deviceTester, SIGNAL(remoteOutput(QString)),
            this, SLOT(processSshOutput(QString)));
    connect(m_deviceTester, SIGNAL(finished()),
            this, SLOT(handleTestThreadFinished()));

    m_deviceTester->start();
}

void MaemoConfigTestDialog::handleTestThreadFinished()
{
    if (!m_deviceTester)
        return;

    QString output;
    if (m_deviceTester->hasError()) {
        output = tr("Device configuration test failed:\n%1").arg(m_deviceTester->error());
        if (m_config.type == MaemoDeviceConfig::Simulator)
            output.append(tr("\nDid you start Qemu?"));
    } else {
        output = parseTestOutput();
    }
    m_ui->testResultEdit->setPlainText(output);
    stopConfigTest();
}

void MaemoConfigTestDialog::stopConfigTest()
{
    if (m_deviceTester) {
        m_deviceTester->disconnect();  // Disconnect signals
        m_deviceTester->stop();

        SafeThreadDeleter *deleter = new SafeThreadDeleter(m_deviceTester);
        deleter->start();

        m_deviceTester = 0;
        m_deviceTestOutput.clear();
        m_closeButton->setText(tr("Close"));
    }
}

void MaemoConfigTestDialog::processSshOutput(const QString &data)
{
    m_deviceTestOutput.append(data);
}

QString MaemoConfigTestDialog::parseTestOutput()
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

} // namespace Internal
} // namespace Qt4ProjectManager
