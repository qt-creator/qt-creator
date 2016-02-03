/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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
        return m_ui->localPathChooser->path();
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
