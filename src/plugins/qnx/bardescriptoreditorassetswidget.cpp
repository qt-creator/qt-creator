/**************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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

    addSignalMapping(BarDescriptorDocument::asset, m_assetsModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)));
    addSignalMapping(BarDescriptorDocument::asset, m_assetsModel, SIGNAL(rowsInserted(QModelIndex,int,int)));
    addSignalMapping(BarDescriptorDocument::asset, m_assetsModel, SIGNAL(rowsRemoved(QModelIndex,int,int)));
}

BarDescriptorEditorAssetsWidget::~BarDescriptorEditorAssetsWidget()
{
    delete m_ui;
}

void BarDescriptorEditorAssetsWidget::clear()
{
    blockSignalMapping(BarDescriptorDocument::asset);
    m_assetsModel->removeRows(0, m_assetsModel->rowCount());
    unblockSignalMapping(BarDescriptorDocument::asset);
}

QStandardItemModel *BarDescriptorEditorAssetsWidget::assetsModel() const
{
    return m_assetsModel;
}

void BarDescriptorEditorAssetsWidget::updateWidgetValue(BarDescriptorDocument::Tag tag, const QVariant &value)
{
    if (tag != BarDescriptorDocument::asset) {
        BarDescriptorEditorAbstractPanelWidget::updateWidgetValue(tag, value);
        return;
    }

    clear();
    BarDescriptorAssetList assets = value.value<BarDescriptorAssetList>();
    foreach (const BarDescriptorAsset asset, assets)
        addAsset(asset);
}

void BarDescriptorEditorAssetsWidget::addAsset(const QString &fullPath)
{
    if (fullPath.isEmpty())
        return;

    BarDescriptorAsset asset;
    asset.source = fullPath;
    asset.destination = QFileInfo(fullPath).fileName();
    asset.entry = false;
    addAsset(asset);
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

void BarDescriptorEditorAssetsWidget::emitChanged(BarDescriptorDocument::Tag tag)
{
    if (tag != BarDescriptorDocument::asset) {
        BarDescriptorEditorAbstractPanelWidget::emitChanged(tag);
        return;
    }

    BarDescriptorAssetList result;
    for (int i = 0; i < m_assetsModel->rowCount(); ++i) {
        BarDescriptorAsset asset;
        asset.source = m_assetsModel->item(i, 0)->text();
        asset.destination = m_assetsModel->item(i, 1)->text();
        asset.entry = m_assetsModel->item(i, 2)->checkState() == Qt::Checked;
        result.append(asset);
    }

    QVariant var;
    var.setValue(result);
    emit changed(tag, var);
}

void BarDescriptorEditorAssetsWidget::addAsset(const BarDescriptorAsset &asset)
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
