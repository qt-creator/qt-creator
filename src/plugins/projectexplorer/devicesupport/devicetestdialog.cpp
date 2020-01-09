/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
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

DeviceTestDialog::DeviceTestDialog(const IDevice::Ptr &deviceConfiguration,
                                   QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<DeviceTestDialogPrivate>(deviceConfiguration->createDeviceTester()))
{
    d->ui.setupUi(this);

    d->deviceTester->setParent(this);
    connect(d->deviceTester, &DeviceTester::progressMessage,
            this, &DeviceTestDialog::handleProgressMessage);
    connect(d->deviceTester, &DeviceTester::errorMessage,
            this, &DeviceTestDialog::handleErrorMessage);
    connect(d->deviceTester, &DeviceTester::finished,
            this, &DeviceTestDialog::handleTestFinished);
    d->deviceTester->testDevice(deviceConfiguration);
}

DeviceTestDialog::~DeviceTestDialog() = default;

void DeviceTestDialog::reject()
{
    if (!d->finished)
        d->deviceTester->stopTest();
    QDialog::reject();
}

void DeviceTestDialog::handleProgressMessage(const QString &message)
{
    addText(message, Utils::Theme::OutputPanes_NormalMessageTextColor, false);
}

void DeviceTestDialog::handleErrorMessage(const QString &message)
{
    addText(message, Utils::Theme::OutputPanes_ErrorMessageTextColor, false);
}

void DeviceTestDialog::handleTestFinished(DeviceTester::TestResult result)
{
    d->finished = true;
    d->ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));

    if (result == DeviceTester::TestSuccess)
        addText(tr("Device test finished successfully."),
                Utils::Theme::OutputPanes_NormalMessageTextColor, true);
    else
        addText(tr("Device test failed."), Utils::Theme::OutputPanes_ErrorMessageTextColor, true);
}

void DeviceTestDialog::addText(const QString &text, Utils::Theme::Color color, bool bold)
{
    Utils::Theme *theme = Utils::creatorTheme();

    QTextCharFormat format = d->ui.textEdit->currentCharFormat();
    format.setForeground(QBrush(theme->color(color)));
    QFont font = format.font();
    font.setBold(bold);
    format.setFont(font);
    d->ui.textEdit->setCurrentCharFormat(format);
    d->ui.textEdit->appendPlainText(text);
}

} // namespace Internal
} // namespace ProjectExplorer
