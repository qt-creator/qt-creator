/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#include "bardescriptoreditorassetswidget.h"
#include "ui_bardescriptoreditorassetswidget.h"

#include "bardescriptordocument.h"

#include <utils/qtcassert.h>

#include <QFileDialog>
#include <QStandardItemModel>

using namespace Qnx;
using namespace Qnx::Internal;

BarDescriptorEditorAssetsWidget::BarDescriptorEditorAssetsWidget(QWidget *parent) :
    BarDescriptorEditorAbstractPanelWidget(parent),
    m_ui(new Ui::BarDescriptorEditorAssetsWidget)
{
    m_ui->setupUi(this);

    QStringList headerLabels;
    headerLabels << tr("Path") << tr("Destination") << tr("Entry-Point");
    m_assetsModel = new QStandardItemModel(this);
    m_assetsModel->setHorizontalHeaderLabels(headerLabels);
    m_ui->assets->setModel(m_assetsModel);

    connect(m_ui->addAsset, SIGNAL(clicked()), this, SLOT(addNewAsset()));
    connect(m_ui->removeAsset, SIGNAL(clicked()), this, SLOT(removeSelectedAsset()));
    connect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
    connectAssetsModel();
}

BarDescriptorEditorAssetsWidget::~BarDescriptorEditorAssetsWidget()
{
    delete m_ui;
}

void BarDescriptorEditorAssetsWidget::clear()
{
    // We can't just block signals, as the view depends on them
    disconnectAssetsModel();
    m_assetsModel->removeRows(0, m_assetsModel->rowCount());
    connectAssetsModel();
}

void BarDescriptorEditorAssetsWidget::addAsset(const BarDescriptorAsset &asset)
{
    disconnectAssetsModel();
    addAssetInternal(asset);
    connectAssetsModel();
}

QList<BarDescriptorAsset> BarDescriptorEditorAssetsWidget::assets() const
{
    QList<BarDescriptorAsset> result;

    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        BarDescriptorAsset asset;
        asset.source = m_assetsModel->item(i, 0)->text();
        asset.destination = m_assetsModel->item(i, 1)->text();
        asset.entry = m_assetsModel->item(i, 2)->checkState() == Qt::Checked;
        result << asset;
    }

    return result;
}

QStandardItemModel *BarDescriptorEditorAssetsWidget::assetsModel() const
{
    return m_assetsModel;
}

void BarDescriptorEditorAssetsWidget::addAsset(const QString &fullPath)
{
    if (fullPath.isEmpty())
        return;

    BarDescriptorAsset asset;
    asset.source = fullPath;
    asset.destination = QFileInfo(fullPath).fileName();
    asset.entry = false;
    addAssetInternal(asset);
}

void BarDescriptorEditorAssetsWidget::removeAsset(const QString &fullPath)
{
    QList<QStandardItem*> assetItems = m_assetsModel->findItems(fullPath);
    foreach (QStandardItem *assetItem, assetItems) {
        QList<QStandardItem*> assetRow = m_assetsModel->takeRow(assetItem->row());
        while (!assetRow.isEmpty())
            delete assetRow.takeLast();
    }
}

void BarDescriptorEditorAssetsWidget::addNewAsset()
{
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Select File to Add"));
    if (fileName.isEmpty())
        return;
    addAsset(fileName);
}

void BarDescriptorEditorAssetsWidget::removeSelectedAsset()
{
    QModelIndexList selectedIndexes = m_ui->assets->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty())
        return;

    foreach (const QModelIndex &index, selectedIndexes)
        m_assetsModel->removeRow(index.row());
}

void BarDescriptorEditorAssetsWidget::updateEntryCheckState(QStandardItem *item)
{
    if (item->column() != 2 || item->checkState() == Qt::Unchecked)
        return;

    disconnect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        QStandardItem *other = m_assetsModel->item(i, 2);
        if (other == item)
            continue;

        // Only one asset can be the entry point
        other->setCheckState(Qt::Unchecked);
    }
    connect(m_assetsModel, SIGNAL(itemChanged(QStandardItem*)), this, SLOT(updateEntryCheckState(QStandardItem*)));
}

void BarDescriptorEditorAssetsWidget::connectAssetsModel()
{
    connect(m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    connect(m_assetsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(changed()));
    connect(m_assetsModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(changed()));
}

void BarDescriptorEditorAssetsWidget::disconnectAssetsModel()
{
    disconnect(m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)), this, SIGNAL(changed()));
    disconnect(m_assetsModel, SIGNAL(rowsInserted(QModelIndex,int,int)), this, SIGNAL(changed()));
    disconnect(m_assetsModel, SIGNAL(rowsRemoved(QModelIndex,int,int)), this, SIGNAL(changed()));
}

void BarDescriptorEditorAssetsWidget::addAssetInternal(const BarDescriptorAsset &asset)
{
    const QString path = asset.source;
    const QString dest = asset.destination;
    QTC_ASSERT(!path.isEmpty(), return);
    QTC_ASSERT(!dest.isEmpty(), return);

    if (hasAsset(asset))
        return;

    QList<QStandardItem *> items;
    items << new QStandardItem(path);
    items << new QStandardItem(dest);

    QStandardItem *entryItem = new QStandardItem();
    entryItem->setCheckable(true);
    entryItem->setCheckState(asset.entry ? Qt::Checked : Qt::Unchecked);
    items << entryItem;
    m_assetsModel->appendRow(items);
}

bool BarDescriptorEditorAssetsWidget::hasAsset(const BarDescriptorAsset &asset)
{
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        QStandardItem *sourceItem = m_assetsModel->item(i, 0);
        QStandardItem *destItem = m_assetsModel->item(i, 1);
        if (sourceItem->text() == asset.source && destItem->text() == asset.destination)
            return true;
    }

    return false;
}
