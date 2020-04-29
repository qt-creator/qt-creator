/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
