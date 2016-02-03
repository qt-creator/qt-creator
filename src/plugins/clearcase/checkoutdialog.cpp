/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
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

#include "checkoutdialog.h"
#include "ui_checkoutdialog.h"
#include "activityselector.h"

#include <QList>
#include <QPair>
#include <QPalette>
#include <QPushButton>

namespace ClearCase {
namespace Internal {

CheckOutDialog::CheckOutDialog(const QString &fileName, bool isUcm, bool showComment,
                               QWidget *parent) :
    QDialog(parent), ui(new Ui::CheckOutDialog)
{
    ui->setupUi(this);
    ui->lblFileName->setText(fileName);

    if (isUcm) {
        m_actSelector = new ActivitySelector(this);

        ui->verticalLayout->insertWidget(0, m_actSelector);

        auto line = new QFrame(this);
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        ui->verticalLayout->insertWidget(1, line);
    }

    if (!showComment)
        hideComment();

    ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();
}

CheckOutDialog::~CheckOutDialog()
{
    delete ui;
}

void CheckOutDialog::hideComment()
{
    ui->lblComment->hide();
    ui->txtComment->hide();
    ui->verticalLayout->invalidate();
    adjustSize();
}

QString CheckOutDialog::activity() const
{
    return m_actSelector ? m_actSelector->activity() : QString();
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
