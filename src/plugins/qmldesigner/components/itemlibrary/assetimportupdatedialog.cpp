// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "assetimportupdatedialog.h"
#include "ui_assetimportupdatedialog.h"
#include "assetimportupdatetreeview.h"
#include "assetimportupdatetreemodel.h"

#include <QDirIterator>
#include <QPushButton>
#include <QTreeView>
#include <QFileInfo>
#include <QModelIndex>

namespace QmlDesigner {
namespace Internal {

AssetImportUpdateDialog::AssetImportUpdateDialog(
        const QString &importPath, const QSet<QString> &preSelectedFiles,
        const QSet<QString> &hiddenEntries, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AssetImportUpdateDialog)
{
    setModal(true);
    ui->setupUi(this);

    connect(ui->buttonBox->button(QDialogButtonBox::Ok), &QPushButton::clicked,
            this, &AssetImportUpdateDialog::accept);
    connect(ui->buttonBox->button(QDialogButtonBox::Cancel), &QPushButton::clicked,
            this, &AssetImportUpdateDialog::reject);
    connect(ui->expandButton, &QPushButton::clicked,
            this, &AssetImportUpdateDialog::expandAll);
    connect(ui->collapseButton, &QPushButton::clicked,
            this, &AssetImportUpdateDialog::collapseAll);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    QList<QFileInfo> infos;
    infos.append(QFileInfo{importPath});
    QDirIterator it(importPath, {"*"}, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        const QString absFile = it.fileInfo().absoluteFilePath();
        if (!hiddenEntries.contains(absFile))
            infos.append(it.fileInfo());
    }

    ui->treeView->model()->createItems(infos, preSelectedFiles);
    ui->treeView->expandAll();
}

AssetImportUpdateDialog::~AssetImportUpdateDialog()
{
    delete ui;
}

QStringList AssetImportUpdateDialog::selectedFiles() const
{
    return ui->treeView->model()->checkedFiles();
}

void AssetImportUpdateDialog::collapseAll()
{
    ui->treeView->collapseAll();
}

void AssetImportUpdateDialog::expandAll()
{
    ui->treeView->expandAll();
}

} // namespace Internal
} // namespace QmlDesigner
