// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "hyperlinkdialog.h"
#include "ui_hyperlinkdialog.h"

#include <QPushButton>

namespace QmlDesigner {


HyperlinkDialog::HyperlinkDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::HyperlinkDialog)
{
    ui->setupUi(this);
    connect (ui->linkEdit, &QLineEdit::textChanged, [this] () {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!(ui->linkEdit->text().isEmpty()));
    });
}

HyperlinkDialog::~HyperlinkDialog()
{
    delete ui;
}

QString HyperlinkDialog::getLink() const
{
    return ui->linkEdit->text().trimmed();
}

void HyperlinkDialog::setLink(const QString &link)
{
    ui->linkEdit->setText(link);
}

QString HyperlinkDialog::getAnchor() const
{
    return ui->anchorEdit->text().trimmed();
}

void HyperlinkDialog::setAnchor(const QString &anchor)
{
    ui->anchorEdit->setText(anchor);
}

}
