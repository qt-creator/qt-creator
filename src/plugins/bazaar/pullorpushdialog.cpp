// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "pullorpushdialog.h"
#include "ui_pullorpushdialog.h"

#include <utils/qtcassert.h>

using namespace Bazaar::Internal;

PullOrPushDialog::PullOrPushDialog(Mode mode, QWidget *parent) : QDialog(parent),
    m_mode(mode),
    m_ui(new Ui::PullOrPushDialog)
{
    m_ui->setupUi(this);
    m_ui->localPathChooser->setExpectedKind(Utils::PathChooser::Directory);
    if (m_mode == PullMode) {
        this->setWindowTitle(tr("Pull Source"));
        m_ui->useExistingDirCheckBox->setVisible(false);
        m_ui->createPrefixCheckBox->setVisible(false);
    } else {
        this->setWindowTitle(tr("Push Destination"));
        m_ui->localCheckBox->setVisible(false);
    }
    this->adjustSize();
}

PullOrPushDialog::~PullOrPushDialog()
{
    delete m_ui;
}

QString PullOrPushDialog::branchLocation() const
{
    if (m_ui->defaultButton->isChecked())
        return QString();
    if (m_ui->localButton->isChecked())
        return m_ui->localPathChooser->filePath().toString();
    return m_ui->urlLineEdit->text();
}

bool PullOrPushDialog::isRememberOptionEnabled() const
{
    if (m_ui->defaultButton->isChecked())
        return false;
    return m_ui->rememberCheckBox->isChecked();
}

bool PullOrPushDialog::isOverwriteOptionEnabled() const
{
    return m_ui->overwriteCheckBox->isChecked();
}

QString PullOrPushDialog::revision() const
{
    return m_ui->revisionLineEdit->text().simplified();
}

bool PullOrPushDialog::isLocalOptionEnabled() const
{
    QTC_ASSERT(m_mode == PullMode, return false);
    return m_ui->localCheckBox->isChecked();
}

bool PullOrPushDialog::isUseExistingDirectoryOptionEnabled() const
{
    QTC_ASSERT(m_mode == PushMode, return false);
    return m_ui->useExistingDirCheckBox->isChecked();
}

bool PullOrPushDialog::isCreatePrefixOptionEnabled() const
{
    QTC_ASSERT(m_mode == PushMode, return false);
    return m_ui->createPrefixCheckBox->isChecked();
}

void PullOrPushDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
