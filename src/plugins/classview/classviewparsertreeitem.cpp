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

#include "classviewparsertreeitem.h"
#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"
#include "classviewconstants.h"
#include "classviewutils.h"

#include <QHash>
#include <QPair>
#include <QIcon>
#include <QStandardItem>
#include <QMutex>

#include <QDebug>

namespace ClassView {
namespace Internal {

///////////////////////////////// ParserTreeItemPrivate //////////////////////////////////

/*!
    \class ParserTreeItemPrivate
    \brief The ParserTreeItemPrivate class defines private class data for
    the ParserTreeItem class.
   \sa ParserTreeItem
 */
class ParserTreeItemPrivate
{
public:
    //! symbol locations
    QSet<SymbolLocation> symbolLocations;

    //! symbol information
    QHash<SymbolInformation, ParserTreeItem::Ptr> symbolInformations;

    //! An icon
    QIcon icon;
};

///////////////////////////////// ParserTreeItem //////////////////////////////////

/*!
    \class ParserTreeItem
    \brief The ParserTreeItem class is an item for the internal Class View tree.

    Not virtual - to speed up its work.
*/

ParserTreeItem::ParserTreeItem() :
    d(new ParserTreeItemPrivate())
{
}

ParserTreeItem::~ParserTreeItem()
{
    delete d;
}

ParserTreeItem &ParserTreeItem::operator=(const ParserTreeItem &other)
{
    d->symbolLocations = other.d->symbolLocations;
    d->icon = other.d->icon;
    d->symbolInformations.clear();
    return *this;
}

/*!
    Copies a parser tree item from the location specified by \a from to this
    item.
*/

void ParserTreeItem::copy(const ParserTreeItem::ConstPtr &from)
{
    if (from.isNull())
        return;

    d->symbolLocations = from->d->symbolLocations;
    d->icon = from->d->icon;
    d->symbolInformations = from->d->symbolInformations;
}

/*!
    \fn void copyTree(const ParserTreeItem::ConstPtr &from)
    Copies a parser tree item with children from the location specified by
    \a from to this item.
*/

void ParserTreeItem::copyTree(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    // copy content
    d->symbolLocations = target->d->symbolLocations;
    d->icon = target->d->icon;
    d->symbolInformations.clear();

    // reserve memory
//    int amount = qMin(100 , target->d_ptr->symbolInformations.count() * 2);
//    d_ptr->symbolInformations.reserve(amount);

    // every child
    CitSymbolInformations cur = target->d->symbolInformations.constBegin();
    CitSymbolInformations end = target->d->symbolInformations.constEnd();

    for (; cur != end; ++cur) {
        ParserTreeItem::Ptr item(new ParserTreeItem());
        item->copyTree(cur.value());
        appendChild(item, cur.key());
    }
}

/*!
    Adds information about symbol location from a \location.
    \sa SymbolLocation, removeSymbolLocation, symbolLocations
*/

void ParserTreeItem::addSymbolLocation(const SymbolLocation &location)
{
    d->symbolLocations.insert(location);
}

/*!
    Adds information about symbol locations from \a locations.
    \sa SymbolLocation, removeSymbolLocation, symbolLocations
*/

void ParserTreeItem::addSymbolLocation(const QSet<SymbolLocation> &locations)
{
    d->symbolLocations.unite(locations);
}

/*!
    Removes information about \a location.
    \sa SymbolLocation, addSymbolLocation, symbolLocations
*/

void ParserTreeItem::removeSymbolLocation(const SymbolLocation &location)
{
    d->symbolLocations.remove(location);
}

/*!
    Removes information about \a locations.
    \sa SymbolLocation, addSymbolLocation, symbolLocations
*/

void ParserTreeItem::removeSymbolLocations(const QSet<SymbolLocation> &locations)
{
    d->symbolLocations.subtract(locations);
}

/*!
    Gets information about symbol positions.
    \sa SymbolLocation, addSymbolLocation, removeSymbolLocation
*/

QSet<SymbolLocation> ParserTreeItem::symbolLocations() const
{
    return d->symbolLocations;
}

/*!
    Appends the child item \a item to \a inf symbol information.
*/

void ParserTreeItem::appendChild(const ParserTreeItem::Ptr &item, const SymbolInformation &inf)
{
    // removeChild must be used to remove an item
    if (item.isNull())
        return;

    d->symbolInformations[inf] = item;
}

/*!
    Removes the \a inf symbol information.
*/

void ParserTreeItem::removeChild(const SymbolInformation &inf)
{
    d->symbolInformations.remove(inf);
}

/*!
    Returns the child item specified by \a inf symbol information.
*/

ParserTreeItem::Ptr ParserTreeItem::child(const SymbolInformation &inf) const
{
    return d->symbolInformations.value(inf);
}

/*!
    Returns the amount of children of the tree item.
*/

int ParserTreeItem::childCount() const
{
    return d->symbolInformations.count();
}

/*!
    \property QIcon::icon
    \brief the icon assigned to the tree item
*/

QIcon ParserTreeItem::icon() const
{
    return d->icon;
}

/*!
    Sets the \a icon for the tree item.
 */
void ParserTreeItem::setIcon(const QIcon &icon)
{
    d->icon = icon;
}

/*!
    Adds an internal state with \a target, which contains the correct current
    state.
*/

void ParserTreeItem::add(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    // add locations
    d->symbolLocations = d->symbolLocations.unite(target->d->symbolLocations);

    // add children
    // every target child
    CitSymbolInformations cur = target->d->symbolInformations.constBegin();
    CitSymbolInformations end = target->d->symbolInformations.constEnd();
    while (cur != end) {
        const SymbolInformation &inf = cur.key();
        const ParserTreeItem::Ptr &targetChild = cur.value();

        ParserTreeItem::Ptr child = d->symbolInformations.value(inf);
        if (!child.isNull()) {
            child->add(targetChild);
        } else {
            ParserTreeItem::Ptr add(new ParserTreeItem());
            add->copyTree(targetChild);
            d->symbolInformations[inf] = add;
        }
        // next item
        ++cur;
    }
}

/*!
    Appends this item to the QStandardIten item \a item.
*/

void ParserTreeItem::convertTo(QStandardItem *item) const
{
    if (!item)
        return;

    QMap<SymbolInformation, ParserTreeItem::Ptr> map;

    // convert to map - to sort it
    CitSymbolInformations curHash = d->symbolInformations.constBegin();
    CitSymbolInformations endHash = d->symbolInformations.constEnd();
    while (curHash != endHash) {
        map.insert(curHash.key(), curHash.value());
        ++curHash;
    }

    typedef QMap<SymbolInformation, ParserTreeItem::Ptr>::const_iterator MapCitSymbolInformations;
    // add to item
    MapCitSymbolInformations cur = map.constBegin();
    MapCitSymbolInformations end = map.constEnd();
    while (cur != end) {
        const SymbolInformation &inf = cur.key();
        ParserTreeItem::Ptr ptr = cur.value();

        QStandardItem *add = new QStandardItem();
        Utils::setSymbolInformationToItem(inf, add);
        if (!ptr.isNull()) {
            // icon
            add->setIcon(ptr->icon());

            // draggable
            if (!ptr->symbolLocations().isEmpty())
                add->setFlags(add->flags() | Qt::ItemIsDragEnabled);

            // locations
            add->setData(Utils::locationsToRole(ptr->symbolLocations()),
                         Constants::SymbolLocationsRole);
        }
        item->appendRow(add);
        ++cur;
    }
}

/*!
    Checks \a item in a QStandardItemModel for lazy data population.
*/

bool ParserTreeItem::canFetchMore(QStandardItem *item) const
{
    if (!item)
        return false;

    int storedChildren = item->rowCount();
    int internalChildren = d->symbolInformations.count();

    if (storedChildren < internalChildren)
        return true;

    return false;
}

/*!
    Performs lazy data population for \a item in a QStandardItemModel if needed.
*/

void ParserTreeItem::fetchMore(QStandardItem *item) const
{
    if (!item)
        return;

    convertTo(item);
}

/*!
    Debug dump.
*/

void ParserTreeItem::debugDump(int ident) const
{
    CitSymbolInformations curHash = d->symbolInformations.constBegin();
    CitSymbolInformations endHash = d->symbolInformations.constEnd();
    while (curHash != endHash) {
        const SymbolInformation &inf = curHash.key();
        qDebug() << QString(2*ident, QLatin1Char(' ')) << inf.iconType() << inf.name() << inf.type()
                << curHash.value().isNull();
        if (!curHash.value().isNull())
            curHash.value()->debugDump(ident + 1);

        ++curHash;
    }
}

} // namespace Internal
} // namespace ClassView

