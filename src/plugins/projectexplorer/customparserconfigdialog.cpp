/****************************************************************************
**
** Copyright (C) 2016 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "customparserconfigdialog.h"
#include "ui_customparserconfigdialog.h"

#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpression>

namespace ProjectExplorer {
namespace Internal {

CustomParserConfigDialog::CustomParserConfigDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::CustomParserConfigDialog)
{
    ui->setupUi(this);

    connect(ui->errorPattern, &QLineEdit::textChanged, this, &CustomParserConfigDialog::changed);
    connect(ui->errorOutputMessage, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(ui->errorFileNameCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);
    connect(ui->errorLineNumberCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);
    connect(ui->errorMessageCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);
    connect(ui->warningPattern, &QLineEdit::textChanged, this, &CustomParserConfigDialog::changed);
    connect(ui->warningOutputMessage, &QLineEdit::textChanged,
            this, &CustomParserConfigDialog::changed);
    connect(ui->warningFileNameCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);
    connect(ui->warningLineNumberCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);
    connect(ui->warningMessageCap, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &CustomParserConfigDialog::changed);

    changed();
    m_dirty = false;
}

CustomParserConfigDialog::~CustomParserConfigDialog()
{
    delete ui;
}

void CustomParserConfigDialog::setExampleSettings()
{
    setErrorPattern(QLatin1String("#error (.*):(\\d+): (.*)"));
    setErrorFileNameCap(1);
    setErrorLineNumberCap(2);
    setErrorMessageCap(3);
    setErrorChannel(CustomParserExpression::ParseBothChannels);
    setWarningPattern(QLatin1String("#warning (.*):(\\d+): (.*)"));
    setWarningFileNameCap(1);
    setWarningLineNumberCap(2);
    setWarningMessageCap(3);
    setWarningChannel(CustomParserExpression::ParseBothChannels);
    ui->errorOutputMessage->setText(
                QLatin1String("#error /home/user/src/test.c:891: Unknown identifier `test`"));
    ui->warningOutputMessage->setText(
                QLatin1String("#warning /home/user/src/test.c:49: Unreferenced variable `test`"));
}

void CustomParserConfigDialog::setSettings(const CustomParserSettings &settings)
{
    if (settings.error.pattern().isEmpty() && settings.warning.pattern().isEmpty()) {
        setExampleSettings();
        return;
    }

    setErrorPattern(settings.error.pattern());
    setErrorFileNameCap(settings.error.fileNameCap());
    setErrorLineNumberCap(settings.error.lineNumberCap());
    setErrorMessageCap(settings.error.messageCap());
    setErrorChannel(settings.error.channel());
    setErrorExample(settings.error.example());
    setWarningPattern(settings.warning.pattern());
    setWarningFileNameCap(settings.warning.fileNameCap());
    setWarningLineNumberCap(settings.warning.lineNumberCap());
    setWarningMessageCap(settings.warning.messageCap());
    setWarningChannel(settings.warning.channel());
    setWarningExample(settings.warning.example());
}

CustomParserSettings CustomParserConfigDialog::settings() const
{
    CustomParserSettings result;
    result.error.setPattern(errorPattern());
    result.error.setFileNameCap(errorFileNameCap());
    result.error.setLineNumberCap(errorLineNumberCap());
    result.error.setMessageCap(errorMessageCap());
    result.error.setChannel(errorChannel());
    result.error.setExample(errorExample());
    result.warning.setPattern(warningPattern());
    result.warning.setFileNameCap(warningFileNameCap());
    result.warning.setLineNumberCap(warningLineNumberCap());
    result.warning.setMessageCap(warningMessageCap());
    result.warning.setChannel(warningChannel());
    result.warning.setExample(warningExample());
    return result;
}

void CustomParserConfigDialog::setErrorPattern(const QString &errorPattern)
{
    ui->errorPattern->setText(errorPattern);
}

QString CustomParserConfigDialog::errorPattern() const
{
    return ui->errorPattern->text();
}

void CustomParserConfigDialog::setErrorFileNameCap(int fileNameCap)
{
    ui->errorFileNameCap->setValue(fileNameCap);
}

int CustomParserConfigDialog::errorFileNameCap() const
{
    return ui->errorFileNameCap->value();
}

void CustomParserConfigDialog::setErrorLineNumberCap(int lineNumberCap)
{
    ui->errorLineNumberCap->setValue(lineNumberCap);
}

int CustomParserConfigDialog::errorLineNumberCap() const
{
    return ui->errorLineNumberCap->value();
}

void CustomParserConfigDialog::setErrorMessageCap(int messageCap)
{
    ui->errorMessageCap->setValue(messageCap);
}

int CustomParserConfigDialog::errorMessageCap() const
{
    return ui->errorMessageCap->value();
}

void CustomParserConfigDialog::setErrorChannel(CustomParserExpression::CustomParserChannel errorChannel)
{
    ui->errorStdErrChannel->setChecked(
                errorChannel & static_cast<int>(CustomParserExpression::ParseStdErrChannel));
    ui->errorStdOutChannel->setChecked(
                errorChannel & static_cast<int>(CustomParserExpression::ParseStdOutChannel));
}

CustomParserExpression::CustomParserChannel CustomParserConfigDialog::errorChannel() const
{
    if (ui->errorStdErrChannel->isChecked() && !ui->errorStdOutChannel->isChecked())
        return CustomParserExpression::ParseStdErrChannel;
    if (ui->errorStdOutChannel->isChecked() && !ui->errorStdErrChannel->isChecked())
        return CustomParserExpression::ParseStdOutChannel;
    return CustomParserExpression::ParseBothChannels;
}

void CustomParserConfigDialog::setErrorExample(const QString &errorExample)
{
    ui->errorOutputMessage->setText(errorExample);
}

QString CustomParserConfigDialog::errorExample() const
{
    return ui->errorOutputMessage->text();
}

void CustomParserConfigDialog::setWarningPattern(const QString &warningPattern)
{
    ui->warningPattern->setText(warningPattern);
}

QString CustomParserConfigDialog::warningPattern() const
{
    return ui->warningPattern->text();
}

void CustomParserConfigDialog::setWarningFileNameCap(int warningFileNameCap)
{
    ui->warningFileNameCap->setValue(warningFileNameCap);
}

int CustomParserConfigDialog::warningFileNameCap() const
{
    return ui->warningFileNameCap->value();
}

void CustomParserConfigDialog::setWarningLineNumberCap(int warningLineNumberCap)
{
    ui->warningLineNumberCap->setValue(warningLineNumberCap);
}

int CustomParserConfigDialog::warningLineNumberCap() const
{
    return ui->warningLineNumberCap->value();
}

void CustomParserConfigDialog::setWarningMessageCap(int warningMessageCap)
{
    ui->warningMessageCap->setValue(warningMessageCap);
}

int CustomParserConfigDialog::warningMessageCap() const
{
    return ui->warningMessageCap->value();
}

void CustomParserConfigDialog::setWarningChannel(CustomParserExpression::CustomParserChannel warningChannel)
{
    ui->warningStdErrChannel->setChecked(
                warningChannel & static_cast<int>(CustomParserExpression::ParseStdErrChannel));
    ui->warningStdOutChannel->setChecked(
                warningChannel & static_cast<int>(CustomParserExpression::ParseStdOutChannel));
}

CustomParserExpression::CustomParserChannel CustomParserConfigDialog::warningChannel() const
{
    if (ui->warningStdErrChannel->isChecked() && !ui->warningStdOutChannel->isChecked())
        return CustomParserExpression::ParseStdErrChannel;
    if (ui->warningStdOutChannel->isChecked() && !ui->warningStdErrChannel->isChecked())
        return CustomParserExpression::ParseStdOutChannel;
    return CustomParserExpression::ParseBothChannels;
}

void CustomParserConfigDialog::setWarningExample(const QString &warningExample)
{
    ui->warningOutputMessage->setText(warningExample);
}

QString CustomParserConfigDialog::warningExample() const
{
    return ui->warningOutputMessage->text();
}

bool CustomParserConfigDialog::isDirty() const
{
    return m_dirty;
}

bool CustomParserConfigDialog::checkPattern(QLineEdit *pattern, const QString &outputText,
                                            QString *errorMessage, QRegularExpressionMatch *match)
{
    QRegularExpression rx;
    rx.setPattern(pattern->text());

    QPalette palette;
    palette.setColor(QPalette::Text, rx.isValid() ? Qt::black : Qt::red);
    pattern->setPalette(palette);
    pattern->setToolTip(rx.isValid() ? QString() : rx.errorString());

    *match = rx.match(outputText);
    if (rx.pattern().isEmpty() || !rx.isValid() || !match->hasMatch()) {
        *errorMessage = QLatin1String("<font color=\"red\">") + tr("Not applicable:") + QLatin1Char(' ');
        if (rx.pattern().isEmpty())
            *errorMessage += tr("Pattern is empty.");
        else if (!rx.isValid())
            *errorMessage += rx.errorString();
        else if (outputText.isEmpty())
            *errorMessage += tr("No message given.");
        else
            *errorMessage += tr("Pattern does not match the message.");

        return false;
    }

    errorMessage->clear();
    return true;
}

void CustomParserConfigDialog::changed()
{
    QRegularExpressionMatch match;
    QString errorMessage;

    if (checkPattern(ui->errorPattern, ui->errorOutputMessage->text(), &errorMessage, &match)) {
        ui->errorFileNameTest->setText(match.captured(ui->errorFileNameCap->value()));
        ui->errorLineNumberTest->setText(match.captured(ui->errorLineNumberCap->value()));
        ui->errorMessageTest->setText(match.captured(ui->errorMessageCap->value()));
    } else {
        ui->errorFileNameTest->setText(errorMessage);
        ui->errorLineNumberTest->setText(errorMessage);
        ui->errorMessageTest->setText(errorMessage);
    }

    if (checkPattern(ui->warningPattern, ui->warningOutputMessage->text(), &errorMessage, &match)) {
        ui->warningFileNameTest->setText(match.captured(ui->warningFileNameCap->value()));
        ui->warningLineNumberTest->setText(match.captured(ui->warningLineNumberCap->value()));
        ui->warningMessageTest->setText(match.captured(ui->warningMessageCap->value()));
    } else {
        ui->warningFileNameTest->setText(errorMessage);
        ui->warningLineNumberTest->setText(errorMessage);
        ui->warningMessageTest->setText(errorMessage);
    }
    m_dirty = true;
}

} // namespace Internal
} // namespace ProjectExplorer
