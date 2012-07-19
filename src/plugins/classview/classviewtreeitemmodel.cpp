/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "classviewtreeitemmodel.h"
#include "classviewconstants.h"
#include "classviewmanager.h"
#include "classviewutils.h"

#include <cplusplus/Icons.h>

namespace ClassView {
namespace Internal {

///////////////////////////////// TreeItemModelPrivate //////////////////////////////////

/*!
   \struct TreeItemModelPrivate
   \brief Private class data for \a TreeItemModel
   \sa TreeItemModel
 */
class TreeItemModelPrivate
{
public:
    //! icon provider
    CPlusPlus::Icons icons;
};

///////////////////////////////// TreeItemModel //////////////////////////////////

TreeItemModel::TreeItemModel(QObject *parent)
    : QStandardItemModel(parent),
    d(new TreeItemModelPrivate())
{
}

TreeItemModel::~TreeItemModel()
{
    delete d;
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
                    return d->icons.iconForType(static_cast<CPlusPlus::Icons::IconType>(type));
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
                name += QLatin1String(" ") + inf.type();

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

void TreeItemModel::moveRootToTarget(const QStandardItem *target)
{
    emit layoutAboutToBeChanged();

    Utils::moveItemToTarget(invisibleRootItem(), target);

    emit layoutChanged();
}

} // namespace Internal
} // namespace ClassView
