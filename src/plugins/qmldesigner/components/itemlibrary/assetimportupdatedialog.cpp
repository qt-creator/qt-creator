/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
