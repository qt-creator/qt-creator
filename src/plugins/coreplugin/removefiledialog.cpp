/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "removefiledialog.h"
#include "ui_removefiledialog.h"

#include <QDir>

using namespace Core;

RemoveFileDialog::RemoveFileDialog(const QString &filePath, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::RemoveFileDialog)
{
    m_ui->setupUi(this);
    m_ui->fileNameLabel->setText(QDir::toNativeSeparators(filePath));

    // TODO
    m_ui->removeVCCheckBox->setVisible(false);
}

RemoveFileDialog::~RemoveFileDialog()
{
    delete m_ui;
}

void RemoveFileDialog::setDeleteFileVisible(bool visible)
{
    m_ui->deleteFileCheckBox->setVisible(visible);
}

bool RemoveFileDialog::isDeleteFileChecked() const
{
    return m_ui->deleteFileCheckBox->isChecked();
}
