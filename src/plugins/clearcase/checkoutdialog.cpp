/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 AudioCodes Ltd.
**
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "checkoutdialog.h"
#include "clearcaseplugin.h"
#include "ui_checkoutdialog.h"

#include <QList>
#include <QPair>
#include <QPalette>

namespace ClearCase {
namespace Internal {

CheckOutDialog::CheckOutDialog(const QString &fileName, QWidget *parent) :
    QDialog(parent), ui(new Ui::CheckOutDialog)
{
    ui->setupUi(this);
    ui->lblFileName->setText(fileName);
}

CheckOutDialog::~CheckOutDialog()
{
    delete ui;
}

QString CheckOutDialog::activity() const
{
    return ui->actSelector->activity();
}

QString CheckOutDialog::comment() const
{
    return ui->txtComment->toPlainText();
}

bool CheckOutDialog::isReserved() const
{
    return ui->chkReserved->isChecked();
}

bool CheckOutDialog::isUnreserved() const
{
    return ui->chkUnreserved->isChecked();
}

bool CheckOutDialog::isPreserveTime() const
{
    return ui->chkPTime->isChecked();
}

bool CheckOutDialog::isUseHijacked() const
{
    return ui->hijackedCheckBox->isChecked();
}

void CheckOutDialog::hideHijack()
{
    ui->hijackedCheckBox->setVisible(false);
    ui->hijackedCheckBox->setChecked(false);
}

void CheckOutDialog::toggleUnreserved(bool checked)
{
    ui->chkUnreserved->setEnabled(checked);
    if (!checked)
        ui->chkUnreserved->setChecked(false);
}

} // namespace Internal
} // namespace ClearCase
