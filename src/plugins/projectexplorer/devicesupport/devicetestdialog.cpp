/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "devicetestdialog.h"
#include "ui_devicetestdialog.h"

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QPushButton>
#include <QTextCharFormat>

namespace ProjectExplorer {
namespace Internal {

class DeviceTestDialog::DeviceTestDialogPrivate
{
public:
    DeviceTestDialogPrivate(DeviceTester *tester)
        : deviceTester(tester), finished(false)
    {
    }

    Ui::DeviceTestDialog ui;
    DeviceTester * const deviceTester;
    bool finished;
};

DeviceTestDialog::DeviceTestDialog(const IDevice::ConstPtr &deviceConfiguration,
                                   QWidget *parent)
    : QDialog(parent), d(new DeviceTestDialogPrivate(deviceConfiguration->createDeviceTester()))
{
    d->ui.setupUi(this);

    d->deviceTester->setParent(this);
    connect(d->deviceTester, SIGNAL(progressMessage(QString)), SLOT(handleProgressMessage(QString)));
    connect(d->deviceTester, SIGNAL(errorMessage(QString)), SLOT(handleErrorMessage(QString)));
    connect(d->deviceTester, SIGNAL(finished(ProjectExplorer::DeviceTester::TestResult)),
        SLOT(handleTestFinished(ProjectExplorer::DeviceTester::TestResult)));
    d->deviceTester->testDevice(deviceConfiguration);
}

DeviceTestDialog::~DeviceTestDialog()
{
    delete d;
}

void DeviceTestDialog::reject()
{
    if (!d->finished)
        d->deviceTester->stopTest();
    QDialog::reject();
}

void DeviceTestDialog::handleProgressMessage(const QString &message)
{
    addText(message, QLatin1String("black"), false);
}

void DeviceTestDialog::handleErrorMessage(const QString &message)
{
    addText(message, QLatin1String("red"), false);
}

void DeviceTestDialog::handleTestFinished(DeviceTester::TestResult result)
{
    d->finished = true;
    d->ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    if (result == DeviceTester::TestSuccess)
        addText(tr("Device test finished successfully."), QLatin1String("blue"), true);
    else
        addText(tr("Device test failed."), QLatin1String("red"), true);
}

void DeviceTestDialog::addText(const QString &text, const QString &color, bool bold)
{
    QTextCharFormat format = d->ui.textEdit->currentCharFormat();
    format.setForeground(QBrush(QColor(color)));
    QFont font = format.font();
    font.setBold(bold);
    format.setFont(font);
    d->ui.textEdit->setCurrentCharFormat(format);
    d->ui.textEdit->appendPlainText(text);
}

} // namespace Internal
} // namespace ProjectExplorer
