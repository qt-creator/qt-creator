// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "puppetdialog.h"
#include "ui_puppetdialog.h"

namespace QmlDesigner {

PuppetDialog::PuppetDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PuppetDialog)
{
    ui->setupUi(this);
}

PuppetDialog::~PuppetDialog()
{
    delete ui;
}

void PuppetDialog::setDescription(const QString &description)
{
    ui->descriptionLabel->setText(description);
}

void PuppetDialog::setCopyAndPasteCode(const QString &text)
{
    ui->copyAndPasteTextEdit->setText(text);
}

void PuppetDialog::warning(QWidget *parent, const QString &title, const QString &description, const QString &copyAndPasteCode)
{
    PuppetDialog dialog(parent);

    dialog.setWindowTitle(title);
    dialog.setDescription(description);
    dialog.setCopyAndPasteCode(copyAndPasteCode);

    dialog.exec();
}

} //QmlDesigner
