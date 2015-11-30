/****************************************************************************
**
** Copyright (C) 2015 Petar Perisin <petar.perisin@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "branchcheckoutdialog.h"
#include "ui_branchcheckoutdialog.h"

namespace Git {
namespace Internal {

BranchCheckoutDialog::BranchCheckoutDialog(QWidget *parent,
                                           const QString &currentBranch,
                                           const QString &nextBranch) :
    QDialog(parent),
    m_ui(new Ui::BranchCheckoutDialog),
    m_foundStashForNextBranch(false),
    m_hasLocalChanges(true)
{
    m_ui->setupUi(this);

    setWindowTitle(tr("Checkout branch \"%1\"").arg(nextBranch));
    m_ui->moveChangesRadioButton->setText(tr("Move Local Changes to \"%1\"").arg(nextBranch));
    m_ui->popStashCheckBox->setText(tr("Pop Stash of \"%1\"").arg(nextBranch));

    if (!currentBranch.isEmpty()) {
        m_ui->makeStashRadioButton->setText(tr("Create Branch Stash for \"%1\"").arg(currentBranch));
    } else {
        m_ui->makeStashRadioButton->setText(tr("Create Branch Stash for Current Branch"));
        foundNoLocalChanges();
    }

    connect(m_ui->moveChangesRadioButton, &QRadioButton::toggled,
            this, &BranchCheckoutDialog::updatePopStashCheckBox);
}

BranchCheckoutDialog::~BranchCheckoutDialog()
{
    delete m_ui;
}

void BranchCheckoutDialog::foundNoLocalChanges()
{
    m_ui->discardChangesRadioButton->setChecked(true);
    m_ui->localChangesGroupBox->setEnabled(false);
    m_hasLocalChanges = false;
}

void BranchCheckoutDialog::foundStashForNextBranch()
{
    m_ui->popStashCheckBox->setChecked(true);
    m_ui->popStashCheckBox->setEnabled(true);
    m_foundStashForNextBranch = true;
}

bool BranchCheckoutDialog::makeStashOfCurrentBranch()
{
    return m_ui->makeStashRadioButton->isChecked();
}

bool BranchCheckoutDialog::moveLocalChangesToNextBranch()
{
    return m_ui->moveChangesRadioButton->isChecked();
}

bool BranchCheckoutDialog::discardLocalChanges()
{
    return m_ui->discardChangesRadioButton->isChecked() && m_ui->localChangesGroupBox->isEnabled();
}

bool BranchCheckoutDialog::popStashOfNextBranch()
{
    return m_ui->popStashCheckBox->isChecked();
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
    m_ui->popStashCheckBox->setChecked(!moveChangesChecked && m_foundStashForNextBranch);
    m_ui->popStashCheckBox->setEnabled(!moveChangesChecked && m_foundStashForNextBranch);
}

} // namespace Internal
} // namespace Git

