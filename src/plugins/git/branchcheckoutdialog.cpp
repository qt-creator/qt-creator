/****************************************************************************
**
** Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
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

#include "branchcheckoutdialog.h"

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>

namespace Git {
namespace Internal {

BranchCheckoutDialog::BranchCheckoutDialog(QWidget *parent,
                                           const QString &currentBranch,
                                           const QString &nextBranch) :
    QDialog(parent)
{
    setWindowModality(Qt::WindowModal);
    resize(394, 199);
    setModal(true);
    setWindowTitle(tr("Checkout branch \"%1\"").arg(nextBranch));

    m_localChangesGroupBox = new QGroupBox(tr("Local Changes Found. Choose Action:"));

    m_moveChangesRadioButton = new QRadioButton(tr("Move Local Changes to \"%1\"").arg(nextBranch));

    m_discardChangesRadioButton = new QRadioButton(tr("Discard Local Changes"));
    m_discardChangesRadioButton->setEnabled(true);

    m_popStashCheckBox = new QCheckBox(tr("Pop Stash of \"%1\"").arg(nextBranch));
    m_popStashCheckBox->setEnabled(false);

    m_makeStashRadioButton = new QRadioButton;
    m_makeStashRadioButton->setChecked(true);
    if (!currentBranch.isEmpty()) {
        m_makeStashRadioButton->setText(tr("Create Branch Stash for \"%1\"").arg(currentBranch));
    } else {
        m_makeStashRadioButton->setText(tr("Create Branch Stash for Current Branch"));
        foundNoLocalChanges();
    }

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Utils::Layouting;

    Column {
        m_makeStashRadioButton,
        m_moveChangesRadioButton,
        m_discardChangesRadioButton
    }.attachTo(m_localChangesGroupBox);

    Column {
        m_localChangesGroupBox,
        m_popStashCheckBox,
        st,
        buttonBox
    }.attachTo(this);

    connect(m_moveChangesRadioButton, &QRadioButton::toggled,
            this, &BranchCheckoutDialog::updatePopStashCheckBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

BranchCheckoutDialog::~BranchCheckoutDialog() = default;

void BranchCheckoutDialog::foundNoLocalChanges()
{
    m_discardChangesRadioButton->setChecked(true);
    m_localChangesGroupBox->setEnabled(false);
    m_hasLocalChanges = false;
}

void BranchCheckoutDialog::foundStashForNextBranch()
{
    m_popStashCheckBox->setChecked(true);
    m_popStashCheckBox->setEnabled(true);
    m_foundStashForNextBranch = true;
}

bool BranchCheckoutDialog::makeStashOfCurrentBranch()
{
    return m_makeStashRadioButton->isChecked();
}

bool BranchCheckoutDialog::moveLocalChangesToNextBranch()
{
    return m_moveChangesRadioButton->isChecked();
}

bool BranchCheckoutDialog::discardLocalChanges()
{
    return m_discardChangesRadioButton->isChecked() && m_localChangesGroupBox->isEnabled();
}

bool BranchCheckoutDialog::popStashOfNextBranch()
{
    return m_popStashCheckBox->isChecked();
}

bool BranchCheckoutDialog::hasStashForNextBranch()
{
    return m_foundStashForNextBranch;
}

bool BranchCheckoutDialog::hasLocalChanges()
{
    return m_hasLocalChanges;
}

void BranchCheckoutDialog::updatePopStashCheckBox(bool moveChangesChecked)
{
    m_popStashCheckBox->setChecked(!moveChangesChecked && m_foundStashForNextBranch);
    m_popStashCheckBox->setEnabled(!moveChangesChecked && m_foundStashForNextBranch);
}

} // namespace Internal
} // namespace Git

