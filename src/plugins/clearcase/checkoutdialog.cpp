// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "checkoutdialog.h"

#include "activityselector.h"
#include "ui_checkoutdialog.h"

#include <utils/layoutbuilder.h>

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
        ui->verticalLayout->insertWidget(1, Utils::Layouting::createHr());
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
