/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "itemlibrarymodel.h"
#include "itemlibrary.h"
#include "customdraganddrop.h"

#include <QtCore/QMimeData>
#include <QtCore/QDebug>

#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtGui/QDrag>
#include <QSortFilterProxyModel>
#include <QPainter>
#include <QLabel>
#include <itemlibraryinfo.h>
#include <QDirModel>


enum { debug = 0 };
// Store data and a type enumeration along with the QStandardItem
enum ItemType { CategoryItem, WidgetItem };
enum Roles { TypeRole = Qt::UserRole + 1,
             DataRole = Qt::UserRole + 2,
             DragPixmapRole = Qt::UserRole + 3};


static inline ItemType itemType(const QStandardItem *item)
{
    return static_cast<ItemType>(item->data(TypeRole).toInt());
}

static inline QmlDesigner::ItemLibraryInfo widgetItemData(const QStandardItem *item)
{
    const QVariant data = item->data(DataRole);
    if (!data.isValid())
        return QmlDesigner::ItemLibraryInfo();
    return qvariant_cast<QmlDesigner::ItemLibraryInfo>(data);
}



namespace QmlDesigner {
namespace Internal {

// Cache a drag pixmap on the icon using the DragPixmapRole data field.
static QImage cachedDragImage(const ItemLibraryInfo &ItemLibraryInfo,
                       QStandardItem *item)
{
    const QVariant cached = item->data(DragPixmapRole);
    if (cached.type() != QVariant::Invalid)
        return qvariant_cast<QImage>(cached);
    // TODO: Grab using factory
    const QIcon icon = ItemLibraryInfo.dragIcon();
    if (icon.isNull())
        return QImage();
    const QList<QSize> sizes = icon.availableSizes();
    if (sizes.isEmpty())
        return QImage();
    const QImage image = icon.pixmap(sizes.front()).toImage();
    item->setData(image, DragPixmapRole);
    return image;
}

ItemLibraryModel::ItemLibraryModel(QObject *parent) :
    QStandardItemModel(parent)
{
    setSupportedDragActions(Qt::CopyAction);
}

static inline QStandardItem *categoryToItem(const QString &g)
{
    QStandardItem *rc = new QStandardItem(g);
    rc->setFlags(Qt::ItemIsEnabled);
    rc->setData(QVariant(CategoryItem), TypeRole);
    rc->setData(g, Qt::UserRole);
    return rc;
}


static QStandardItem *customWidgetDataToItem(const ItemLibraryInfo &ItemLibraryInfo)
{
    QStandardItem *item = new QStandardItem(ItemLibraryInfo.name());
    const QIcon icon = ItemLibraryInfo.icon();
    if (!icon.isNull() && !icon.availableSizes().empty()) {
        item->setIcon(icon);
        if (icon.availableSizes().count() == 1) {
            item->setSizeHint(icon.availableSizes().first() + QSize(1, 1));
        }
    }

    item->setFlags(Qt::ItemIsEnabled|Qt::ItemIsDragEnabled|Qt::ItemIsSelectable);
    item->setData(qVariantFromValue(ItemLibraryInfo), DataRole);
    item->setData(QVariant(WidgetItem), TypeRole);
    item->setData(ItemLibraryInfo.name(), Qt::UserRole);
    return item;
}


void ItemLibraryModel::addItemLibraryInfo(const ItemLibraryInfo &itemLibraryInfo)
{
    QStandardItem *categoryItem = findCategoryItem(itemLibraryInfo.category());
    if (!categoryItem) {
        categoryItem = categoryToItem(itemLibraryInfo.category());
        appendRow(categoryItem);
    }
    categoryItem->appendRow(customWidgetDataToItem(itemLibraryInfo));
    QString filterList = categoryItem->data(Qt::UserRole).toString();
    filterList += itemLibraryInfo.name();
    categoryItem->setData(filterList, Qt::UserRole);
}

QStandardItem *ItemLibraryModel::findCategoryItem(const QString &category)
{
    const QStandardItem *root = invisibleRootItem();
    const int rowCount = root->rowCount();
    for (int i = 0 ; i < rowCount; i++) {
        QStandardItem *categoryItem = root->child(i, 0);
        if (categoryItem->text() == category)
            return categoryItem;
    }
    return 0;
}

Qt::DropActions ItemLibraryModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

Qt::DropActions ItemLibraryModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}

QStringList ItemLibraryModel::mimeTypes () const
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    return QStringList(QLatin1String("text/xml"));
}

QByteArray ItemLibraryInfoToByteArray(const ItemLibraryInfo &ItemLibraryInfo)
{
    QByteArray byteArray;
    QDataStream stream(&byteArray, QIODevice::WriteOnly);

    stream << ItemLibraryInfo;

    return byteArray;
}

QMimeData *ItemLibraryModel::mimeData(const QModelIndexList &indexes) const
{
    if (debug)
        qDebug() << Q_FUNC_INFO << indexes.size();
    if (indexes.size() != 1)
        return 0;
    QStandardItem *item = itemFromIndex (indexes.front());
    if (!item || itemType(item) != WidgetItem)
        return 0;
    QMimeData *mimeData = new QMimeData;

    ItemLibraryInfo ItemLibraryInfo(widgetItemData(item));

    const QImage image = cachedDragImage(ItemLibraryInfo, item);
    if (!image.isNull())
        mimeData->setImageData(image);


    mimeData->setData("application/vnd.bauhaus.itemlibraryinfo", ItemLibraryInfoToByteArray(ItemLibraryInfo));
    mimeData->removeFormat("text/plain");

    return mimeData;
}

ItemLibraryTreeView::ItemLibraryTreeView(QWidget *parent) :
        QTreeView(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(activateItem(const QModelIndex &)));
}

// We need to implement startDrag ourselves since we cannot
// otherwise influence drag pixmap and hotspot in the standard
// implementation.
void ItemLibraryTreeView::startDrag(Qt::DropActions /* supportedActions */)
{
    if (debug)
        qDebug() << Q_FUNC_INFO;
    QMimeData *mimeData = model()->mimeData(selectedIndexes());
    if (!mimeData)
        return;

    if (qobject_cast<QSortFilterProxyModel*>(model())) {
        QModelIndex index = qobject_cast<QSortFilterProxyModel*>(model())->mapToSource(selectedIndexes().front());

        QStandardItem *item = m_model->itemFromIndex(index);

        if (!item)
            return;

        CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
        const QImage image = qvariant_cast<QImage>(mimeData->imageData());
        drag->setPixmap(item->icon().pixmap(32, 32));
        drag->setPreview(QPixmap::fromImage(image));
        drag->setMimeData(mimeData);

        drag->exec();
    } else {
        QDirModel *dirModel = qobject_cast<QDirModel*>(model());
        Q_ASSERT(dirModel);
        QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
        QPixmap pixmap(fileInfo.absoluteFilePath());
        if (!pixmap.isNull()) {
            CustomItemLibraryDrag *drag = new CustomItemLibraryDrag(this);
            drag->setPreview(pixmap);
            drag->setPixmap(QIcon(pixmap).pixmap(128, 128));
            QMimeData *mimeData = new QMimeData;
            mimeData->setData("application/vnd.bauhaus.libraryresource", fileInfo.absoluteFilePath().toLatin1());
            drag->setMimeData(mimeData);
            drag->exec();
        }
    }
}

static ItemLibraryInfo ItemLibraryInfoFromData(const QByteArray &data)
{
    QDataStream stream(data);

    ItemLibraryInfo itemLibraryInfo;
    stream >> itemLibraryInfo;

    return itemLibraryInfo;
}

void ItemLibraryTreeView::activateItem( const QModelIndex & /*index*/)
{
    QMimeData *mimeData = model()->mimeData(selectedIndexes());
    if (!mimeData)
        return;

    QString name;
    if (qobject_cast<QSortFilterProxyModel*>(model())) {
        QModelIndex index = qobject_cast<QSortFilterProxyModel*>(model())->mapToSource(selectedIndexes().front());

        QStandardItem *item = m_model->itemFromIndex(index);

        if (!item)
            return;

        ItemLibraryInfo itemLibraryInfo = ItemLibraryInfoFromData(mimeData->data("application/vnd.bauhaus.itemlibraryinfo"));
        QString type = itemLibraryInfo.name();

        name = "item^" + type;
        emit itemActivated(name);
    } else {
        QDirModel *dirModel = qobject_cast<QDirModel*>(model());
        Q_ASSERT(dirModel);
        QFileInfo fileInfo = dirModel->fileInfo(selectedIndexes().front());
        QPixmap pixmap(fileInfo.absoluteFilePath());
        if (!pixmap.isNull()) {
            name = "image^" + fileInfo.absoluteFilePath();
            emit itemActivated(name);
        }
    }
}

} // namespace Internal
} // namespace QmlDesigner
