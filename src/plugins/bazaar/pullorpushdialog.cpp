/**************************************************************************
**
** Copyright (c) 2013 Hugues Delorme
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "pullorpushdialog.h"
#include "ui_pullorpushdialog.h"

#include <utils/qtcassert.h>

using namespace Bazaar::Internal;

PullOrPushDialog::PullOrPushDialog(Mode mode, QWidget *parent)
    : QDialog(parent),
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
