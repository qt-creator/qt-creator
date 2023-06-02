// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openwithdialog.h"

#include "../coreplugintr.h"

#include <utils/filepath.h>
#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QListWidget>
#include <QPushButton>

namespace Core::Internal {

OpenWithDialog::OpenWithDialog(const Utils::FilePath &filePath, QWidget *parent)
    : QDialog(parent)
    , editorListWidget(new QListWidget)
    , buttonBox(new QDialogButtonBox)
{
    resize(358, 199);
    setWindowTitle(Tr::tr("Open File With..."));

    buttonBox->setStandardButtons(QDialogButtonBox::Cancel | QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    using namespace Layouting;
    // clang-format off
    Column {
        Tr::tr("Open file \"%1\" with:").arg(filePath.fileName()),
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

} // Core::Internal
