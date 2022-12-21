// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewtreeitemmodel.h"

#include "classviewconstants.h"
#include "classviewmanager.h"
#include "classviewutils.h"

#include <cplusplus/Icons.h>
#include <utils/dropsupport.h>

namespace ClassView {
namespace Internal {

/*!
   Moves \a item to \a target (sorted).
*/

static void moveItemToTarget(QStandardItem *item, const QStandardItem *target)
{
    if (!item || !target)
        return;

    int itemIndex = 0;
    int targetIndex = 0;
    int itemRows = item->rowCount();
    int targetRows = target->rowCount();

    while (itemIndex < itemRows && targetIndex < targetRows) {
        QStandardItem *itemChild = item->child(itemIndex);
        const QStandardItem *targetChild = target->child(targetIndex);

        const SymbolInformation &itemInf = Internal::symbolInformationFromItem(itemChild);
        const SymbolInformation &targetInf = Internal::symbolInformationFromItem(targetChild);

        if (itemInf < targetInf) {
            item->removeRow(itemIndex);
            --itemRows;
        } else if (itemInf == targetInf) {
            moveItemToTarget(itemChild, targetChild);
            ++itemIndex;
            ++targetIndex;
        } else {
            item->insertRow(itemIndex, targetChild->clone());
            moveItemToTarget(item->child(itemIndex), targetChild);
            ++itemIndex;
            ++itemRows;
            ++targetIndex;
        }
    }

    // append
    while (targetIndex < targetRows) {
        item->appendRow(target->child(targetIndex)->clone());
        moveItemToTarget(item->child(itemIndex), target->child(targetIndex));
        ++itemIndex;
        ++itemRows;
        ++targetIndex;
    }

    // remove end of item
    while (itemIndex < itemRows) {
        item->removeRow(itemIndex);
        --itemRows;
    }
}

///////////////////////////////// TreeItemModel //////////////////////////////////

/*!
   \class TreeItemModel
   \brief The TreeItemModel class provides a model for the Class View tree.
*/

TreeItemModel::TreeItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

TreeItemModel::~TreeItemModel() = default;

QVariant TreeItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QStandardItemModel::data(index, role);

    switch (role) {
    case Qt::DecorationRole: {
            QVariant iconType = data(index, Constants::IconTypeRole);
            if (iconType.isValid()) {
                bool ok = false;
                int type = iconType.toInt(&ok);
                if (ok && type >= 0)
                    return ::Utils::CodeModelIcon::iconForType(static_cast<::Utils::CodeModelIcon::Type>(type));
            }
        }
        break;
    case Qt::ToolTipRole:
    case Qt::DisplayRole: {
            const SymbolInformation &inf = Internal::symbolInformationFromItem(itemFromIndex(index));

            if (inf.name() == inf.type() || inf.iconType() < 0)
                return inf.name();

            QString name(inf.name());

            if (!inf.type().isEmpty())
                name += QLatin1Char(' ') + inf.type();

            return name;
        }
        break;
    default:
        break;
    }

    return QStandardItemModel::data(index, role);
}

bool TreeItemModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return false;

    return Manager::instance()->canFetchMore(itemFromIndex(parent));
}

void TreeItemModel::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid())
        return;

    return Manager::instance()->fetchMore(itemFromIndex(parent));
}

bool TreeItemModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;

    return Manager::instance()->hasChildren(itemFromIndex(parent));
}

Qt::DropActions TreeItemModel::supportedDragActions() const
{
    return Qt::MoveAction | Qt::CopyAction;
}

QStringList TreeItemModel::mimeTypes() const
{
    return ::Utils::DropSupport::mimeTypesForFilePaths();
}

QMimeData *TreeItemModel::mimeData(const QModelIndexList &indexes) const
{
    auto mimeData = new ::Utils::DropMimeData;
    mimeData->setOverrideFileDropAction(Qt::CopyAction);
    for (const QModelIndex &index : indexes) {
        const QSet<SymbolLocation> locations = Internal::roleToLocations(
                    data(index, Constants::SymbolLocationsRole).toList());
        if (locations.isEmpty())
            continue;
        const SymbolLocation loc = *locations.constBegin();
        mimeData->addFile(loc.filePath(), loc.line(), loc.column());
    }
    if (mimeData->files().isEmpty()) {
        delete mimeData;
        return nullptr;
    }
    return mimeData;
}

/*!
   Moves the root item to the \a target item.
*/

void TreeItemModel::moveRootToTarget(const QStandardItem *target)
{
    emit layoutAboutToBeChanged();

    moveItemToTarget(invisibleRootItem(), target);

    emit layoutChanged();
}

} // namespace Internal
} // namespace ClassView
