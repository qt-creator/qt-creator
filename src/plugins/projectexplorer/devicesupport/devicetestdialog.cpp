// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicetestdialog.h"

#include "../projectexplorertr.h"

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>

#include <QBrush>
#include <QColor>
#include <QDialogButtonBox>
#include <QFont>
#include <QPlainTextEdit>
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

    DeviceTester * const deviceTester;
    bool finished;
    QPlainTextEdit *textEdit;
    QDialogButtonBox *buttonBox;
};

DeviceTestDialog::DeviceTestDialog(const IDevice::Ptr &deviceConfiguration,
                                   QWidget *parent)
    : QDialog(parent)
    , d(std::make_unique<DeviceTestDialogPrivate>(deviceConfiguration->createDeviceTester()))
{
    resize(620, 580);
    d->textEdit = new QPlainTextEdit;
    d->textEdit->setReadOnly(true);
    d->buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel);

    using namespace Layouting;
    Column {
        d->textEdit,
        d->buttonBox,
    }.attachTo(this);

    d->deviceTester->setParent(this);
    connect(d->buttonBox, &QDialogButtonBox::rejected, this, &DeviceTestDialog::reject);
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
    if (!d->finished) {
        d->deviceTester->disconnect(this);
        d->deviceTester->stopTest();
    }
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
    d->buttonBox->button(QDialogButtonBox::Cancel)->setText(Tr::tr("Close"));

    if (result == DeviceTester::TestSuccess)
        addText(Tr::tr("Device test finished successfully."),
                Utils::Theme::OutputPanes_NormalMessageTextColor, true);
    else
        addText(Tr::tr("Device test failed."), Utils::Theme::OutputPanes_ErrorMessageTextColor, true);
}

void DeviceTestDialog::addText(const QString &text, Utils::Theme::Color color, bool bold)
{
    Utils::Theme *theme = Utils::creatorTheme();

    QTextCharFormat format = d->textEdit->currentCharFormat();
    format.setForeground(QBrush(theme->color(color)));
    QFont font = format.font();
    font.setBold(bold);
    format.setFont(font);
    d->textEdit->setCurrentCharFormat(format);
    d->textEdit->appendPlainText(text);
}

} // namespace Internal
} // namespace ProjectExplorer
