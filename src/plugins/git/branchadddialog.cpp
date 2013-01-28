/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "branchadddialog.h"
#include "ui_branchadddialog.h"

namespace Git {
namespace Internal {

BranchAddDialog::BranchAddDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::BranchAddDialog)
{
    m_ui->setupUi(this);
}

BranchAddDialog::~BranchAddDialog()
{
    delete m_ui;
}

void BranchAddDialog::setBranchName(const QString &n)
{
    m_ui->branchNameEdit->setText(n);
    m_ui->branchNameEdit->selectAll();
}

QString BranchAddDialog::branchName() const
{
    return m_ui->branchNameEdit->text();
}

void BranchAddDialog::setTrackedBranchName(const QString &name, bool remote)
{
    m_ui->trackingCheckBox->setVisible(true);
    if (!name.isEmpty())
        m_ui->trackingCheckBox->setText(remote ? tr("Track remote branch \'%1\'").arg(name) :
                                                 tr("Track local branch \'%1\'").arg(name));
    else
        m_ui->trackingCheckBox->setVisible(false);
    m_ui->trackingCheckBox->setChecked(remote);
}

bool BranchAddDialog::track()
{
    return m_ui->trackingCheckBox->isChecked();
}

} // namespace Internal
} // namespace Git
