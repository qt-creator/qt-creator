// Copyright (C) 2016 Denis Mingulov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "classviewparsertreeitem.h"

#include "classviewconstants.h"

#include <cplusplus/Icons.h>
#include <cplusplus/Overview.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectmanager.h>

#include <QDebug>
#include <QHash>
#include <QStandardItem>

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
    ParserTreeItem::ConstPtr cloneTree() const;

    QHash<SymbolInformation, ParserTreeItem::ConstPtr> m_symbolInformations;
    QSet<SymbolLocation> m_symbolLocations;
    const Utils::FilePath m_projectFilePath;
};

void ParserTreeItemPrivate::mergeWith(const ParserTreeItem::ConstPtr &target)
{
    if (target.isNull())
        return;

    m_symbolLocations.unite(target->d->m_symbolLocations);

    // merge children
    for (auto it = target->d->m_symbolInformations.cbegin();
              it != target->d->m_symbolInformations.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        const ParserTreeItem::ConstPtr &targetChild = it.value();

        ParserTreeItem::ConstPtr child = m_symbolInformations.value(inf);
        if (!child.isNull()) {
            child->d->mergeWith(targetChild);
        } else {
            const ParserTreeItem::ConstPtr clone = targetChild.isNull() ? ParserTreeItem::ConstPtr()
                                                                   : targetChild->d->cloneTree();
            m_symbolInformations.insert(inf, clone);
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
    if (symbol->asForwardClassDeclaration()
        || symbol->isExtern()
        || symbol->isFriend()
        || symbol->isGenerated()
        || symbol->asUsingNamespaceDirective()
        || symbol->asUsingDeclaration()
        )
        return;

    const CPlusPlus::Name *symbolName = symbol->name();
    if (symbolName && symbolName->asQualifiedNameId())
        return;

    QString name = g_overview.prettyName(symbolName).trimmed();
    QString type = g_overview.prettyType(symbol->type()).trimmed();
    int iconType = CPlusPlus::Icons::iconTypeForSymbol(symbol);

    SymbolInformation information(name, type, iconType);

    // If next line will be removed, 5% speed up for the initial parsing.
    // But there might be a problem for some files ???
    // Better to improve qHash timing
    ParserTreeItem::ConstPtr childItem = m_symbolInformations.value(information);

    if (childItem.isNull())
        childItem = ParserTreeItem::ConstPtr(new ParserTreeItem());

    // locations have 1-based column in Symbol, use the same here.
    SymbolLocation location(symbol->filePath(),
                            symbol->line(), symbol->column());

    childItem->d->m_symbolLocations.insert(location);

    // prevent showing a content of the functions
    if (!symbol->asFunction()) {
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
    if (!symbol->asNamespace() || childItem->childCount())
        m_symbolInformations.insert(information, childItem);
}

/*!
    Creates a deep clone of this tree.
*/
ParserTreeItem::ConstPtr ParserTreeItemPrivate::cloneTree() const
{
    ParserTreeItem::ConstPtr newItem(new ParserTreeItem(m_projectFilePath));
    newItem->d->m_symbolLocations = m_symbolLocations;

    for (auto it = m_symbolInformations.cbegin(); it != m_symbolInformations.cend(); ++it) {
        ParserTreeItem::ConstPtr child = it.value();
        if (child.isNull())
            continue;
        newItem->d->m_symbolInformations.insert(it.key(), child->d->cloneTree());
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

ParserTreeItem::ParserTreeItem(const Utils::FilePath &projectFilePath)
    : d(new ParserTreeItemPrivate({{}, {}, projectFilePath}))
{
}

ParserTreeItem::ParserTreeItem(const QHash<SymbolInformation, ConstPtr> &children)
    : d(new ParserTreeItemPrivate({children, {}, {}}))
{
}

ParserTreeItem::~ParserTreeItem()
{
    delete d;
}

Utils::FilePath ParserTreeItem::projectFilePath() const
{
    return d->m_projectFilePath;
}

/*!
    Gets information about symbol positions.
    \sa SymbolLocation, addSymbolLocation, removeSymbolLocation
*/

QSet<SymbolLocation> ParserTreeItem::symbolLocations() const
{
    return d->m_symbolLocations;
}

/*!
    Returns the child item specified by \a inf symbol information.
*/

ParserTreeItem::ConstPtr ParserTreeItem::child(const SymbolInformation &inf) const
{
    return d->m_symbolInformations.value(inf);
}

/*!
    Returns the amount of children of the tree item.
*/

int ParserTreeItem::childCount() const
{
    return d->m_symbolInformations.count();
}

ParserTreeItem::ConstPtr ParserTreeItem::parseDocument(const CPlusPlus::Document::Ptr &doc)
{
    ConstPtr item(new ParserTreeItem());

    const unsigned total = doc->globalSymbolCount();
    for (unsigned i = 0; i < total; ++i)
        item->d->mergeSymbol(doc->globalSymbolAt(i));

    return item;
}

ParserTreeItem::ConstPtr ParserTreeItem::mergeTrees(const Utils::FilePath &projectFilePath,
                                               const QList<ConstPtr> &docTrees)
{
    ConstPtr item(new ParserTreeItem(projectFilePath));
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
    Checks \a item in a QStandardItemModel for lazy data population.
    Make sure this method is called only from the GUI thread.
*/
bool ParserTreeItem::canFetchMore(QStandardItem *item) const
{
    if (!item)
        return false;
    return item->rowCount() < d->m_symbolInformations.count();
}

/*!
    Appends this item to the QStandardIten item \a item.
    Make sure this method is called only from the GUI thread.
*/
void ParserTreeItem::fetchMore(QStandardItem *item) const
{
    using ProjectExplorer::ProjectManager;
    if (!item)
        return;

    // convert to map - to sort it
    QMap<SymbolInformation, ConstPtr> map;
    for (auto it = d->m_symbolInformations.cbegin(); it != d->m_symbolInformations.cend(); ++it)
        map.insert(it.key(), it.value());

    for (auto it = map.cbegin(); it != map.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        ConstPtr ptr = it.value();

        auto add = new QStandardItem;
        add->setData(inf.name(), Constants::SymbolNameRole);
        add->setData(inf.type(), Constants::SymbolTypeRole);
        add->setData(inf.iconType(), Constants::IconTypeRole);

        if (!ptr.isNull()) {
            // icon
            const Utils::FilePath &filePath = ptr->projectFilePath();
            if (!filePath.isEmpty()) {
                ProjectExplorer::Project *project = ProjectManager::projectForFile(filePath);
                if (project)
                    add->setIcon(project->containerNode()->icon());
            }

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
    Debug dump.
*/

void ParserTreeItem::debugDump(int indent) const
{
    for (auto it = d->m_symbolInformations.cbegin(); it != d->m_symbolInformations.cend(); ++it) {
        const SymbolInformation &inf = it.key();
        const ConstPtr &child = it.value();
        qDebug() << QString(2 * indent, QLatin1Char(' ')) << inf.iconType() << inf.name()
                 << inf.type() << child.isNull();
        if (!child.isNull())
            child->debugDump(indent + 1);
    }
}

} // namespace Internal
} // namespace ClassView

