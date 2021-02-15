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

#include <cplusplus/Icons.h>
#include <cplusplus/Name.h>
#include <cplusplus/Overview.h>
#include <cplusplus/Symbol.h>
#include <cplusplus/Symbols.h>
#include <utils/algorithm.h>

#include <QHash>
#include <QPair>
#include <QIcon>
#include <QStandardItem>
#include <QMutex>

#include <QDebug>

namespace ClassView {
namespace Internal {

static CPlusPlus::Overview g_overview;

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
    void mergeWith(const ParserTreeItem::ConstPtr &target);
    void mergeSymbol(const CPlusPlus::Symbol *symbol);
    ParserTreeItem::Ptr cloneTree() const;

    QHash<SymbolInformation, ParserTreeItem::Ptr> symbolInformations;
    QSet<SymbolLocation> symbolLocations;
    QIcon icon;
};

void ParserTreeItemPrivate::mergeWith(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    symbolLocations.unite(target->d->symbolLocations);

    // merge children
    for (auto it = target->d->symbolInformations.cbegin();
              it != target->d->symbolInformations.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        const ParserTreeItem::Ptr &targetChild = it.value();

        ParserTreeItem::Ptr child = symbolInformations.value(inf);
        if (!child.isNull()) {
            child->d->mergeWith(targetChild);
        } else {
            const ParserTreeItem::Ptr clone = targetChild.isNull() ? ParserTreeItem::Ptr()
                                                                   : targetChild->d->cloneTree();
            symbolInformations.insert(inf, clone);
        }
    }
}

void ParserTreeItemPrivate::mergeSymbol(const CPlusPlus::Symbol *symbol)
{
    if (!symbol)
        return;

    // easy solution - lets add any scoped symbol and
    // any symbol which does not contain :: in the name

    //! \todo collect statistics and reorder to optimize
    if (symbol->isForwardClassDeclaration()
        || symbol->isExtern()
        || symbol->isFriend()
        || symbol->isGenerated()
        || symbol->isUsingNamespaceDirective()
        || symbol->isUsingDeclaration()
        )
        return;

    const CPlusPlus::Name *symbolName = symbol->name();
    if (symbolName && symbolName->isQualifiedNameId())
        return;

    QString name = g_overview.prettyName(symbolName).trimmed();
    QString type = g_overview.prettyType(symbol->type()).trimmed();
    int iconType = CPlusPlus::Icons::iconTypeForSymbol(symbol);

    SymbolInformation information(name, type, iconType);

    // If next line will be removed, 5% speed up for the initial parsing.
    // But there might be a problem for some files ???
    // Better to improve qHash timing
    ParserTreeItem::Ptr childItem = symbolInformations.value(information);

    if (childItem.isNull())
        childItem = ParserTreeItem::Ptr(new ParserTreeItem());

    // locations have 1-based column in Symbol, use the same here.
    SymbolLocation location(QString::fromUtf8(symbol->fileName() , symbol->fileNameLength()),
                            symbol->line(), symbol->column());

    childItem->d->symbolLocations.insert(location);

    // prevent showing a content of the functions
    if (!symbol->isFunction()) {
        if (const CPlusPlus::Scope *scope = symbol->asScope()) {
            CPlusPlus::Scope::iterator cur = scope->memberBegin();
            CPlusPlus::Scope::iterator last = scope->memberEnd();
            while (cur != last) {
                const CPlusPlus::Symbol *curSymbol = *cur;
                ++cur;
                if (!curSymbol)
                    continue;

                childItem->d->mergeSymbol(curSymbol);
            }
        }
    }

    // if item is empty and has not to be added
    if (!symbol->isNamespace() || childItem->childCount())
        symbolInformations.insert(information, childItem);
}

/*!
    Creates a deep clone of this tree.
*/
ParserTreeItem::Ptr ParserTreeItemPrivate::cloneTree() const
{
    ParserTreeItem::Ptr newItem(new ParserTreeItem);
    newItem->d->symbolLocations = symbolLocations;
    newItem->d->icon = icon;

    for (auto it = symbolInformations.cbegin(); it != symbolInformations.cend(); ++it) {
        ParserTreeItem::ConstPtr child = it.value();
        if (child.isNull())
            continue;
        newItem->d->symbolInformations.insert(it.key(), child->d->cloneTree());
    }

    return newItem;
}

///////////////////////////////// ParserTreeItem //////////////////////////////////

/*!
    \class ParserTreeItem
    \brief The ParserTreeItem class is an item for the internal Class View tree.

    Not virtual - to speed up its work.
*/

ParserTreeItem::ParserTreeItem()
    : d(new ParserTreeItemPrivate())
{
}

ParserTreeItem::ParserTreeItem(const QHash<SymbolInformation, Ptr> &children)
    : d(new ParserTreeItemPrivate({children, {}, {}}))
{
}


ParserTreeItem::~ParserTreeItem()
{
    delete d;
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

ParserTreeItem::Ptr ParserTreeItem::parseDocument(const CPlusPlus::Document::Ptr &doc)
{
    Ptr item(new ParserTreeItem());

    const unsigned total = doc->globalSymbolCount();
    for (unsigned i = 0; i < total; ++i)
        item->d->mergeSymbol(doc->globalSymbolAt(i));

    return item;
}

ParserTreeItem::Ptr ParserTreeItem::mergeTrees(const QList<ConstPtr> &docTrees)
{
    Ptr item(new ParserTreeItem());
    for (const ConstPtr &docTree : docTrees)
        item->d->mergeWith(docTree);

    return item;
}

/*!
    Converts internal location container to QVariant compatible.
    \a locations specifies a set of symbol locations.
    Returns a list of variant locations that can be added to the data of an
    item.
*/

static QList<QVariant> locationsToRole(const QSet<SymbolLocation> &locations)
{
    QList<QVariant> locationsVar;
    for (const SymbolLocation &loc : locations)
        locationsVar.append(QVariant::fromValue(loc));

    return locationsVar;
}

/*!
    Appends this item to the QStandardIten item \a item.
*/

void ParserTreeItem::convertTo(QStandardItem *item) const
{
    if (!item)
        return;

    // convert to map - to sort it
    QMap<SymbolInformation, Ptr> map;
    for (auto it = d->symbolInformations.cbegin(); it != d->symbolInformations.cend(); ++it)
        map.insert(it.key(), it.value());

    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        Ptr ptr = it.value();

        auto add = new QStandardItem;
        add->setData(inf.name(), Constants::SymbolNameRole);
        add->setData(inf.type(), Constants::SymbolTypeRole);
        add->setData(inf.iconType(), Constants::IconTypeRole);

        if (!ptr.isNull()) {
            // icon
            add->setIcon(ptr->icon());

            // draggable
            if (!ptr->symbolLocations().isEmpty())
                add->setFlags(add->flags() | Qt::ItemIsDragEnabled);

            // locations
            add->setData(locationsToRole(ptr->symbolLocations()), Constants::SymbolLocationsRole);
        }
        item->appendRow(add);
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
    return storedChildren < internalChildren;
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

void ParserTreeItem::debugDump(int indent) const
{
    for (auto it = d->symbolInformations.cbegin(); it != d->symbolInformations.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        const Ptr &child = it.value();
        qDebug() << QString(2 * indent, QLatin1Char(' ')) << inf.iconType() << inf.name()
                 << inf.type() << child.isNull();
        if (!child.isNull())
            child->debugDump(indent + 1);
    }
}

} // namespace Internal
} // namespace ClassView

