// Copyright (C) 2016 Petar Perisin <petar.perisin@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "branchcheckoutdialog.h"

#include "gittr.h"

#include <utils/layoutbuilder.h>

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>

namespace Git::Internal {

BranchCheckoutDialog::BranchCheckoutDialog(QWidget *parent,
                                           const QString &currentBranch,
                                           const QString &nextBranch) :
    QDialog(parent)
{
    setWindowModality(Qt::WindowModal);
    resize(394, 199);
    setModal(true);
    setWindowTitle(Tr::tr("Checkout branch \"%1\"").arg(nextBranch));

    m_localChangesGroupBox = new QGroupBox(Tr::tr("Local Changes Found. Choose Action:"));

    m_moveChangesRadioButton = new QRadioButton(Tr::tr("Move Local Changes to \"%1\"").arg(nextBranch));

    m_discardChangesRadioButton = new QRadioButton(Tr::tr("Discard Local Changes"));
    m_discardChangesRadioButton->setEnabled(true);

    m_popStashCheckBox = new QCheckBox(Tr::tr("Pop Stash of \"%1\"").arg(nextBranch));
    m_popStashCheckBox->setEnabled(false);

    m_makeStashRadioButton = new QRadioButton;
    m_makeStashRadioButton->setChecked(true);
    if (!currentBranch.isEmpty()) {
        m_makeStashRadioButton->setText(Tr::tr("Create Branch Stash for \"%1\"").arg(currentBranch));
    } else {
        m_makeStashRadioButton->setText(Tr::tr("Create Branch Stash for Current Branch"));
        foundNoLocalChanges();
    }

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

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

} // Git::Internal
