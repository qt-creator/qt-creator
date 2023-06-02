// Copyright (C) 2016 AudioCodes Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "checkoutdialog.h"

#include "activityselector.h"
#include "clearcasetr.h"

#include <utils/layoutbuilder.h>

#include <QAbstractButton>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace ClearCase::Internal {

CheckOutDialog::CheckOutDialog(const QString &fileName, bool isUcm, bool showComment,
                               QWidget *parent) :
    QDialog(parent)
{
    resize(352, 317);
    setWindowTitle(Tr::tr("Check Out"));

    auto lblFileName = new QLabel(fileName);
    lblFileName->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::TextSelectableByMouse);

    m_txtComment = new QPlainTextEdit(this);
    m_txtComment->setTabChangesFocus(true);

    m_lblComment = new QLabel(Tr::tr("&Checkout comment:"));
    m_lblComment->setBuddy(m_txtComment);

    m_chkReserved = new QCheckBox(Tr::tr("&Reserved"));
    m_chkReserved->setChecked(true);

    m_chkUnreserved = new QCheckBox(Tr::tr("&Unreserved if already reserved"));

    m_chkPTime = new QCheckBox(Tr::tr("&Preserve file modification time"));

    m_hijackedCheckBox = new QCheckBox(Tr::tr("Use &Hijacked file"));
    m_hijackedCheckBox->setChecked(true);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        lblFileName,
        m_lblComment,
        m_txtComment,
        m_chkReserved,
        Row { Space(16), m_chkUnreserved },
        m_chkPTime,
        m_hijackedCheckBox,
        buttonBox
    }.attachTo(this);

    m_verticalLayout = static_cast<QVBoxLayout *>(layout());

    if (isUcm) {
        m_actSelector = new ActivitySelector(this);

        m_verticalLayout->insertWidget(0, m_actSelector);
        m_verticalLayout->insertWidget(1, Layouting::createHr());
    }

    if (!showComment)
        hideComment();

    buttonBox->button(QDialogButtonBox::Ok)->setFocus();

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_chkReserved, &QAbstractButton::toggled, this, &CheckOutDialog::toggleUnreserved);
}

CheckOutDialog::~CheckOutDialog() = default;

void CheckOutDialog::hideComment()
{
    m_lblComment->hide();
    m_txtComment->hide();
    m_verticalLayout->invalidate();
    adjustSize();
}

QString CheckOutDialog::activity() const
{
    return m_actSelector ? m_actSelector->activity() : QString();
}

QString CheckOutDialog::comment() const
{
    return m_txtComment->toPlainText();
}

bool CheckOutDialog::isReserved() const
{
    return m_chkReserved->isChecked();
}

bool CheckOutDialog::isUnreserved() const
{
    return m_chkUnreserved->isChecked();
}

bool CheckOutDialog::isPreserveTime() const
{
    return m_chkPTime->isChecked();
}

bool CheckOutDialog::isUseHijacked() const
{
    return m_hijackedCheckBox->isChecked();
}

void CheckOutDialog::hideHijack()
{
    m_hijackedCheckBox->setVisible(false);
    m_hijackedCheckBox->setChecked(false);
}

void CheckOutDialog::toggleUnreserved(bool checked)
{
    m_chkUnreserved->setEnabled(checked);
    if (!checked)
        m_chkUnreserved->setChecked(false);
}

} // ClearCase::Internal
