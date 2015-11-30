/****************************************************************************
**
** Copyright (C) 2015 Andre Hartmann.
** Contact: aha_1980@gmx.de
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

#include "customparserconfigdialog.h"
#include "ui_customparserconfigdialog.h"

#include <QPushButton>
#include <QRegExp>

namespace ProjectExplorer {
namespace Internal {

CustomParserConfigDialog::CustomParserConfigDialog(QDialog *parent) :
    QDialog(parent),
    ui(new Ui::CustomParserConfigDialog)
{
    ui->setupUi(this);

    connect(ui->errorPattern, SIGNAL(textChanged(QString)), this, SLOT(changed()));
    connect(ui->errorMessage, SIGNAL(textChanged(QString)), this, SLOT(changed()));
    connect(ui->fileNameCap, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(ui->lineNumberCap, SIGNAL(valueChanged(int)), this, SLOT(changed()));
    connect(ui->messageCap, SIGNAL(valueChanged(int)), this, SLOT(changed()));

    changed();
    m_dirty = false;
}

CustomParserConfigDialog::~CustomParserConfigDialog()
{
    delete ui;
}

void CustomParserConfigDialog::setExampleSettings()
{
    setErrorPattern(QLatin1String("#error (.*):(\\d+): (.*)$"));
    setFileNameCap(1);
    setLineNumberCap(2);
    setMessageCap(3);
    ui->errorMessage->setText(QLatin1String("#error /home/user/src/test.c:891: Unknown identifier `test`"));
}

void CustomParserConfigDialog::setSettings(const CustomParserSettings &settings)
{
    if (settings.errorPattern.isEmpty()) {
        setExampleSettings();
        return;
    }

    setErrorPattern(settings.errorPattern);
    setFileNameCap(settings.fileNameCap);
    setLineNumberCap(settings.lineNumberCap);
    setMessageCap(settings.messageCap);
}

CustomParserSettings CustomParserConfigDialog::settings() const
{
    CustomParserSettings result;
    result.errorPattern = errorPattern();
    result.fileNameCap = fileNameCap();
    result.lineNumberCap = lineNumberCap();
    result.messageCap = messageCap();
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

void CustomParserConfigDialog::setFileNameCap(int fileNameCap)
{
    ui->fileNameCap->setValue(fileNameCap);
}

int CustomParserConfigDialog::fileNameCap() const
{
    return ui->fileNameCap->value();
}

void CustomParserConfigDialog::setLineNumberCap(int lineNumberCap)
{
    ui->lineNumberCap->setValue(lineNumberCap);
}

int CustomParserConfigDialog::lineNumberCap() const
{
    return ui->lineNumberCap->value();
}

void CustomParserConfigDialog::setMessageCap(int messageCap)
{
    ui->messageCap->setValue(messageCap);
}

int CustomParserConfigDialog::messageCap() const
{
    return ui->messageCap->value();
}

bool CustomParserConfigDialog::isDirty() const
{
    return m_dirty;
}

void CustomParserConfigDialog::changed()
{
    QRegExp rx;
    rx.setPattern(ui->errorPattern->text());
    rx.setMinimal(true);

    QPalette palette;
    palette.setColor(QPalette::Text, rx.isValid() ? Qt::black : Qt::red);
    ui->errorPattern->setPalette(palette);
    ui->errorPattern->setToolTip(rx.isValid() ? QString() : rx.errorString());

    int pos = rx.indexIn(ui->errorMessage->text());
    if (rx.isEmpty() || !rx.isValid() || pos < 0) {
        QString error = QLatin1String("<font color=\"red\">") + tr("Not applicable:") + QLatin1Char(' ');
        if (rx.isEmpty())
            error += tr("Pattern is empty.");
        else if (!rx.isValid())
            error += rx.errorString();
        else
            error += tr("Pattern does not match the error message.");

        ui->fileNameTest->setText(error);
        ui->lineNumberTest->setText(error);
        ui->messageTest->setText(error);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        return;
    }

    ui->fileNameTest->setText(rx.cap(ui->fileNameCap->value()));
    ui->lineNumberTest->setText(rx.cap(ui->lineNumberCap->value()));
    ui->messageTest->setText(rx.cap(ui->messageCap->value()));
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    m_dirty = true;
}

} // namespace Internal
} // namespace ProjectExplorer
