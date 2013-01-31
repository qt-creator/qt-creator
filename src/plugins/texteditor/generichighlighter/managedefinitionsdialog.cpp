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

#include "managedefinitionsdialog.h"
#include "manager.h"

#include <QUrl>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>

#include <QDebug>

using namespace TextEditor;
using namespace Internal;

ManageDefinitionsDialog::ManageDefinitionsDialog(
        const QList<HighlightDefinitionMetaData> &metaDataList,
        const QString &path,
        QWidget *parent) :
    QDialog(parent),
    m_definitionsMetaData(metaDataList),
    m_path(path)
{
    ui.setupUi(this);
    ui.definitionsTable->setHorizontalHeaderLabels(
        QStringList() << tr("Name") << tr("Installed") << tr("Available"));
    ui.definitionsTable->horizontalHeader()->setResizeMode(0, QHeaderView::Stretch);

    setWindowTitle(tr("Download Definitions"));

    populateDefinitionsWidget();

    connect(ui.downloadButton, SIGNAL(clicked()), this, SLOT(downloadDefinitions()));
    connect(ui.allButton, SIGNAL(clicked()), this, SLOT(selectAll()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearSelection()));
    connect(ui.invertButton, SIGNAL(clicked()), this, SLOT(invertSelection()));
}

void ManageDefinitionsDialog::populateDefinitionsWidget()
{
    const int size = m_definitionsMetaData.size();
    ui.definitionsTable->setRowCount(size);
    for (int i = 0; i < size; ++i) {
        const HighlightDefinitionMetaData &downloadData = m_definitionsMetaData.at(i);

        // Look for this definition in the current path specified by the user, not the one
        // stored in the settings. So the manager should not be queried for this information.
        QString dirVersion;
        QFileInfo fi(m_path + downloadData.fileName());
        QFile definitionFile(fi.absoluteFilePath());
        if (definitionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QSharedPointer<HighlightDefinitionMetaData> &data = Manager::parseMetadata(fi);
            if (!data.isNull())
                dirVersion = data->version();
        }

        for (int j = 0; j < 3; ++j) {
            QTableWidgetItem *item = new QTableWidgetItem;
            if (j == 0)
                item->setText(downloadData.name());
            else if (j == 1) {
                item->setText(dirVersion);
                item->setTextAlignment(Qt::AlignCenter);
            } else if (j == 2) {
                item->setText(downloadData.version());
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
    foreach (const QModelIndex &index, ui.definitionsTable->selectionModel()->selectedRows())
        urls.append(m_definitionsMetaData.at(index.row()).url());
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

void ManageDefinitionsDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }
}
