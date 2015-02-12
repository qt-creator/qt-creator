/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "managedefinitionsdialog.h"
#include "manager.h"

#include <QUrl>
#include <QIODevice>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

using namespace TextEditor;
using namespace Internal;

ManageDefinitionsDialog::ManageDefinitionsDialog(
        const QList<DefinitionMetaDataPtr> &metaDataList,
        const QString &path,
        QWidget *parent) :
    QDialog(parent),
    m_path(path)
{
    ui.setupUi(this);
    ui.definitionsTable->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Installed") << tr("Available"));
    ui.definitionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);

    setWindowTitle(tr("Download Definitions"));

    populateDefinitionsWidget(metaDataList);
    ui.definitionsTable->sortItems(0);

    connect(ui.downloadButton, &QPushButton::clicked, this, &ManageDefinitionsDialog::downloadDefinitions);
    connect(ui.allButton, &QPushButton::clicked, this, &ManageDefinitionsDialog::selectAll);
    connect(ui.clearButton, &QPushButton::clicked, this, &ManageDefinitionsDialog::clearSelection);
    connect(ui.invertButton, &QPushButton::clicked, this, &ManageDefinitionsDialog::invertSelection);
}

void ManageDefinitionsDialog::populateDefinitionsWidget(const QList<DefinitionMetaDataPtr> &definitionsMetaData)
{
    const int size = definitionsMetaData.size();
    ui.definitionsTable->setRowCount(size);
    for (int i = 0; i < size; ++i) {
        const HighlightDefinitionMetaData &downloadData = *definitionsMetaData.at(i);

        // Look for this definition in the current path specified by the user, not the one
        // stored in the settings. So the manager should not be queried for this information.
        QString dirVersion;
        QFileInfo fi(m_path + downloadData.fileName);
        QFile definitionFile(fi.absoluteFilePath());
        if (definitionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const DefinitionMetaDataPtr data = Manager::parseMetadata(fi);
            if (!data.isNull())
                dirVersion = data->version;
        }

        for (int j = 0; j < 3; ++j) {
            QTableWidgetItem *item = new QTableWidgetItem;
            if (j == 0) {
                item->setText(downloadData.name);
                item->setData(Qt::UserRole, downloadData.url);
            } else if (j == 1) {
                item->setText(dirVersion);
                item->setTextAlignment(Qt::AlignCenter);
            } else if (j == 2) {
                item->setText(downloadData.version);
                item->setTextAlignment(Qt::AlignCenter);
            }
            ui.definitionsTable->setItem(i, j, item);
        }
    }
}

void ManageDefinitionsDialog::downloadDefinitions()
{
    if (Manager::instance()->isDownloadingDefinitions()) {
        QMessageBox::information(
            this,
            tr("Download Information"),
            tr("There is already one download in progress. Please wait until it is finished."));
        return;
    }

    QList<QUrl> urls;
    foreach (const QModelIndex &index, ui.definitionsTable->selectionModel()->selectedRows()) {
        const QVariant url = ui.definitionsTable->item(index.row(), 0)->data(Qt::UserRole);
        urls.append(url.toUrl());
    }
    Manager::instance()->downloadDefinitions(urls, m_path);
    accept();
}

void ManageDefinitionsDialog::selectAll()
{
    ui.definitionsTable->selectAll();
    ui.definitionsTable->setFocus();
}

void ManageDefinitionsDialog::clearSelection()
{
    ui.definitionsTable->clearSelection();
}

void ManageDefinitionsDialog::invertSelection()
{
    const QModelIndex &topLeft = ui.definitionsTable->model()->index(0, 0);
    const QModelIndex &bottomRight =
        ui.definitionsTable->model()->index(ui.definitionsTable->rowCount() - 1,
                                            ui.definitionsTable->columnCount() - 1);
    QItemSelection itemSelection(topLeft, bottomRight);
    ui.definitionsTable->selectionModel()->select(itemSelection, QItemSelectionModel::Toggle);
    ui.definitionsTable->setFocus();
}
