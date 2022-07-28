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

#include "openwithdialog.h"

#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>

using namespace Core;
using namespace Core::Internal;

OpenWithDialog::OpenWithDialog(const Utils::FilePath &filePath, QWidget *parent)
    : QDialog(parent)
    , editorListWidget(new QListWidget)
    , buttonBox(new QDialogButtonBox)
{
    resize(358, 199);
    setWindowTitle(tr("Open File With..."));

    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    using namespace Utils::Layouting;
    // clang-format off
    Column {
        tr("Open file \"%1\" with:").arg(filePath.fileName()),
        editorListWidget,
        buttonBox
    }.attachTo(this);
    // clang-format on

    connect(buttonBox->button(QDialogButtonBox::Ok), &QAbstractButton::clicked,
            this, &QDialog::accept);
    connect(buttonBox->button(QDialogButtonBox::Cancel), &QAbstractButton::clicked,
            this, &QDialog::reject);
    connect(editorListWidget, &QListWidget::itemDoubleClicked,
        this, &QDialog::accept);
    connect(editorListWidget, &QListWidget::currentItemChanged,
            this, &OpenWithDialog::currentItemChanged);

    setOkButtonEnabled(false);
}

void OpenWithDialog::setOkButtonEnabled(bool v)
{
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(v);
}

void OpenWithDialog::setEditors(const QStringList &editors)
{
    for (const QString &e : editors)
        editorListWidget->addItem(e);
}

int OpenWithDialog::editor() const
{
    return editorListWidget->currentRow();
}

void OpenWithDialog::setCurrentEditor(int index)
{
    editorListWidget->setCurrentRow(index);
}

void OpenWithDialog::currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    setOkButtonEnabled(current);
}
