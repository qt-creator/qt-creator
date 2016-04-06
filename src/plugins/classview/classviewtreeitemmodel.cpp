/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include "classviewtreeitemmodel.h"
#include "classviewconstants.h"
#include "classviewmanager.h"
#include "classviewutils.h"

#include <cplusplus/Icons.h>
#include <utils/dropsupport.h>

namespace ClassView {
namespace Internal {

///////////////////////////////// TreeItemModel //////////////////////////////////

/*!
   \class TreeItemModel
   \brief The TreeItemModel class provides a model for the Class View tree.
*/

TreeItemModel::TreeItemModel(QObject *parent)
    : QStandardItemModel(parent)
{
}

TreeItemModel::~TreeItemModel()
{
}

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
                    return CPlusPlus::Icons::iconForType(static_cast<CPlusPlus::Icons::IconType>(type));
            }
        }
        break;
    case Qt::ToolTipRole:
    case Qt::DisplayRole: {
            const SymbolInformation &inf = Utils::symbolInformationFromItem(itemFromIndex(index));

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
    foreach (const QModelIndex &index, indexes) {
        const QSet<SymbolLocation> locations = Utils::roleToLocations(
                    data(index, Constants::SymbolLocationsRole).toList());
        if (locations.isEmpty())
            continue;
        const SymbolLocation loc = *locations.constBegin();
        mimeData->addFile(loc.fileName(), loc.line(), loc.column());
    }
    if (mimeData->files().isEmpty()) {
        delete mimeData;
        return 0;
    }
    return mimeData;
}

/*!
   Moves the root item to the \a target item.
*/

void TreeItemModel::moveRootToTarget(const QStandardItem *target)
{
    emit layoutAboutToBeChanged();

    Utils::moveItemToTarget(invisibleRootItem(), target);

    emit layoutChanged();
}

} // namespace Internal
} // namespace ClassView
