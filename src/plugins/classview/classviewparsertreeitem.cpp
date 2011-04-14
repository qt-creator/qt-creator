/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Denis Mingulov.
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "classviewparsertreeitem.h"
#include "classviewsymbollocation.h"
#include "classviewsymbolinformation.h"
#include "classviewconstants.h"
#include "classviewutils.h"

#include <QtCore/QHash>
#include <QtCore/QPair>
#include <QtGui/QIcon>
#include <QtGui/QStandardItem>
#include <QtCore/QMutex>

#include <QtCore/QDebug>

enum { debug = true };

namespace ClassView {
namespace Internal {

///////////////////////////////// ParserTreeItemPrivate //////////////////////////////////

/*!
   \struct ParserTreeItemPrivate
   \brief Private class data for \a ParserTreeItem
   \sa ParserTreeItem
 */
struct ParserTreeItemPrivate
{
    //! symbol locations
    QSet<SymbolLocation> symbolLocations;

    //! symbol information
    QHash<SymbolInformation, ParserTreeItem::Ptr> symbolInformations;

    //! An icon
    QIcon icon;
};

///////////////////////////////// ParserTreeItem //////////////////////////////////

ParserTreeItem::ParserTreeItem() :
    d_ptr(new ParserTreeItemPrivate())
{
}

ParserTreeItem::~ParserTreeItem()
{
}

ParserTreeItem &ParserTreeItem::operator=(const ParserTreeItem &other)
{
    d_ptr->symbolLocations = other.d_ptr->symbolLocations;
    d_ptr->icon = other.d_ptr->icon;
    d_ptr->symbolInformations.clear();
    return *this;
}

void ParserTreeItem::copy(const ParserTreeItem::ConstPtr &from)
{
    if (from.isNull())
        return;

    d_ptr->symbolLocations = from->d_ptr->symbolLocations;
    d_ptr->icon = from->d_ptr->icon;
    d_ptr->symbolInformations = from->d_ptr->symbolInformations;
}

void ParserTreeItem::copyTree(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    // copy content
    d_ptr->symbolLocations = target->d_ptr->symbolLocations;
    d_ptr->icon = target->d_ptr->icon;
    d_ptr->symbolInformations.clear();

    // reserve memory
//    int amount = qMin(100 , target->d_ptr->symbolInformations.count() * 2);
//    d_ptr->symbolInformations.reserve(amount);

    // every child
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator cur =
            target->d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator end =
            target->d_ptr->symbolInformations.constEnd();

    for(; cur != end; cur++) {
        ParserTreeItem::Ptr item(new ParserTreeItem());
        item->copyTree(cur.value());
        appendChild(item, cur.key());
    }
}

void ParserTreeItem::addSymbolLocation(const SymbolLocation &location)
{
    d_ptr->symbolLocations.insert(location);
}

void ParserTreeItem::addSymbolLocation(const QSet<SymbolLocation> &locations)
{
    d_ptr->symbolLocations.unite(locations);
}

void ParserTreeItem::removeSymbolLocation(const SymbolLocation &location)
{
    d_ptr->symbolLocations.remove(location);
}

void ParserTreeItem::removeSymbolLocations(const QSet<SymbolLocation> &locations)
{
    d_ptr->symbolLocations.subtract(locations);
}

QSet<SymbolLocation> ParserTreeItem::symbolLocations() const
{
    return d_ptr->symbolLocations;
}

void ParserTreeItem::appendChild(const ParserTreeItem::Ptr &item, const SymbolInformation &inf)
{
    // removeChild must be used to remove an item
    if (item.isNull())
        return;

    d_ptr->symbolInformations[inf] = item;
}

void ParserTreeItem::removeChild(const SymbolInformation &inf)
{
    d_ptr->symbolInformations.remove(inf);
}

ParserTreeItem::Ptr ParserTreeItem::child(const SymbolInformation &inf) const
{
    if (!d_ptr->symbolInformations.contains(inf))
        return ParserTreeItem::Ptr();
    return d_ptr->symbolInformations[inf];
}

int ParserTreeItem::childCount() const
{
    return d_ptr->symbolInformations.count();
}

QIcon ParserTreeItem::icon() const
{
    return d_ptr->icon;
}

void ParserTreeItem::setIcon(const QIcon &icon)
{
    d_ptr->icon = icon;
}

void ParserTreeItem::add(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    // add locations
    d_ptr->symbolLocations = d_ptr->symbolLocations.unite(target->d_ptr->symbolLocations);

    // add children
    // every target child
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator cur =
            target->d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator end =
            target->d_ptr->symbolInformations.constEnd();
    while(cur != end) {
        const SymbolInformation &inf = cur.key();
        const ParserTreeItem::Ptr &targetChild = cur.value();
        if (d_ptr->symbolInformations.contains(inf)) {
            // this item has the same child node
            const ParserTreeItem::Ptr &child = d_ptr->symbolInformations[inf];
            if (!child.isNull()) {
                child->add(targetChild);
            } else {
                ParserTreeItem::Ptr add(new ParserTreeItem());
                add->copyTree(targetChild);
                d_ptr->symbolInformations[inf] = add;
            }
        } else {
            ParserTreeItem::Ptr add(new ParserTreeItem());
            add->copyTree(targetChild);
            d_ptr->symbolInformations[inf] = add;
        }
        // next item
        ++cur;
    }
}

void ParserTreeItem::subtract(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    // every target child
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator cur =
            target->d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator end =
            target->d_ptr->symbolInformations.constEnd();
    while(cur != end) {
        const SymbolInformation &inf = cur.key();
        if (d_ptr->symbolInformations.contains(inf)) {
            // this item has the same child node
            if (!d_ptr->symbolInformations[inf].isNull())
                d_ptr->symbolInformations[inf]->subtract(cur.value());
            if (d_ptr->symbolInformations[inf].isNull()
                || d_ptr->symbolInformations[inf]->childCount() == 0)
                d_ptr->symbolInformations.remove(inf);
        }
        // next item
        ++cur;
    }
}

void ParserTreeItem::convertTo(QStandardItem *item, bool recursive) const
{
    if (!item)
        return;

    QMap<SymbolInformation, ParserTreeItem::Ptr> map;

    // convert to map - to sort it
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator curHash =
            d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator endHash =
            d_ptr->symbolInformations.constEnd();
    while(curHash != endHash) {
        map.insert(curHash.key(), curHash.value());
        ++curHash;
    }

    // add to item
    QMap<SymbolInformation, ParserTreeItem::Ptr>::const_iterator cur = map.constBegin();
    QMap<SymbolInformation, ParserTreeItem::Ptr>::const_iterator end = map.constEnd();
    while(cur != end) {
        const SymbolInformation &inf = cur.key();
        ParserTreeItem::Ptr ptr = cur.value();

        QStandardItem *add = new QStandardItem();
        Utils::setSymbolInformationToItem(inf, add);
        if (!ptr.isNull()) {
            // icon
            add->setIcon(ptr->icon());

            // locations
            add->setData(Utils::locationsToRole(ptr->symbolLocations()),
                         Constants::SymbolLocationsRole);

            if (recursive)
                cur.value()->convertTo(add, false);
        }
        item->appendRow(add);
        ++cur;
    }
}

bool ParserTreeItem::canFetchMore(QStandardItem *item) const
{
    if (!item)
        return false;

    // incremental data population - so we have to check children
    // count subchildren of both - current QStandardItem and our internal

    // for the current UI item
    int storedChildren = 0;
    for (int i = 0; i < item->rowCount(); i++) {
        QStandardItem *child = item->child(i);
        if (!child)
            continue;
        storedChildren += child->rowCount();
    }
    // children for the internal state
    int internalChildren = 0;
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator curHash =
            d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator endHash =
            d_ptr->symbolInformations.constEnd();
    while(curHash != endHash) {
        const ParserTreeItem::Ptr &child = curHash.value();
        if (!child.isNull()) {
            internalChildren += child->childCount();
            // if there is already more items than stored, then can be stopped right now
            if (internalChildren > storedChildren)
                break;
        }
        ++curHash;
    }

    if(storedChildren < internalChildren)
        return true;

    return false;
}

void ParserTreeItem::fetchMore(QStandardItem *item) const
{
    if (!item)
        return;

    for (int i = 0; i < item->rowCount(); i++) {
        QStandardItem *child = item->child(i);
        if (!child)
            continue;

        const SymbolInformation &childInf = Utils::symbolInformationFromItem(child);

        if (d_ptr->symbolInformations.contains(childInf)) {
            const ParserTreeItem::Ptr &childPtr = d_ptr->symbolInformations[childInf];
            if (childPtr.isNull())
                continue;

            // create a standard
            QScopedPointer<QStandardItem> state(new QStandardItem());
            childPtr->convertTo(state.data(), false);

            Utils::fetchItemToTarget(child, state.data());
        }
    }
}

void ParserTreeItem::debugDump(int ident) const
{
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator curHash =
            d_ptr->symbolInformations.constBegin();
    QHash<SymbolInformation, ParserTreeItem::Ptr>::const_iterator endHash =
            d_ptr->symbolInformations.constEnd();
    while(curHash != endHash) {
        const SymbolInformation &inf = curHash.key();
        qDebug() << QString(2*ident, QChar(' ')) << inf.iconType() << inf.name() << inf.type()
                << curHash.value().isNull();
        if (!curHash.value().isNull())
            curHash.value()->debugDump(ident + 1);

        ++curHash;
    }
}

} // namespace Internal
} // namespace ClassView

