/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "searchsymbols.h"

#include <cplusplus/Icons.h>
#include <cplusplus/LookupContext.h>
#include <utils/qtcassert.h>
#include <utils/scopedswap.h>

#include <QDebug>

using namespace CPlusPlus;

namespace CppTools {

typedef Utils::ScopedSwap<IndexItem::Ptr> ScopedIndexItemPtr;
typedef Utils::ScopedSwap<QString> ScopedScope;

SearchSymbols::SymbolTypes SearchSymbols::AllTypes =
        SymbolSearcher::Classes
        | SymbolSearcher::Functions
        | SymbolSearcher::Enums
        | SymbolSearcher::Declarations;

SearchSymbols::SearchSymbols(Internal::StringTable &stringTable)
    : strings(stringTable)
    , symbolsToSearchFor(SymbolSearcher::Classes | SymbolSearcher::Functions | SymbolSearcher::Enums)
{
}

void SearchSymbols::setSymbolsToSearchFor(const SymbolTypes &types)
{
    symbolsToSearchFor = types;
}

IndexItem::Ptr SearchSymbols::operator()(Document::Ptr doc, const QString &scope)
{
    IndexItem::Ptr root = IndexItem::create(findOrInsert(doc->fileName()), 100);

    { // RAII scope
        ScopedIndexItemPtr parentRaii(_parent, root);
        QString newScope = scope;
        ScopedScope scopeRaii(_scope, newScope);

        QTC_ASSERT(_parent, return IndexItem::Ptr());
        QTC_ASSERT(root, return IndexItem::Ptr());
        QTC_ASSERT(_parent->fileName() == findOrInsert(doc->fileName()),
                   return IndexItem::Ptr());

        for (unsigned i = 0, ei = doc->globalSymbolCount(); i != ei; ++i)
            accept(doc->globalSymbolAt(i));

        strings.scheduleGC();
        m_paths.clear();
    }

    root->squeeze();
    return root;
}

bool SearchSymbols::visit(Enum *symbol)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Enums))
        return false;

    QString name = overview.prettyName(symbol->name());
    IndexItem::Ptr newParent = addChildItem(name, QString(), _scope, IndexItem::Enum, symbol);
    if (!newParent)
        newParent = _parent;
    ScopedIndexItemPtr parentRaii(_parent, newParent);

    QString newScope = scopedSymbolName(name, symbol);
    ScopedScope scopeRaii(_scope, newScope);

    for (unsigned i = 0, ei = symbol->memberCount(); i != ei; ++i)
        accept(symbol->memberAt(i));

    return false;
}

bool SearchSymbols::visit(Function *symbol)
{
    processFunction(symbol);
    return false;
}

bool SearchSymbols::visit(Namespace *symbol)
{
    QString name = scopedSymbolName(symbol);
    QString newScope = name;
    ScopedScope raii(_scope, newScope);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    return false;
}

bool SearchSymbols::visit(Declaration *symbol)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Declarations)) {
        // if we're searching for functions, still allow signal declarations to show up.
        if (symbolsToSearchFor & SymbolSearcher::Functions) {
            Function *funTy = symbol->type()->asFunctionType();
            if (!funTy) {
                if (!symbol->type()->asObjCMethodType())
                    return false;
            } else if (!funTy->isSignal()) {
                return false;
            }
        } else {
            return false;
        }
    }

    if (symbol->name()) {
        QString name = overview.prettyName(symbol->name());
        QString type = overview.prettyType(symbol->type());
        addChildItem(name, type, _scope,
                     symbol->type()->asFunctionType() ? IndexItem::Function
                                                      : IndexItem::Declaration,
                     symbol);
    }

    return false;
}

bool SearchSymbols::visit(Class *symbol)
{
    processClass(symbol);

    return false;
}

bool SearchSymbols::visit(UsingNamespaceDirective *)
{
    return false;
}

bool SearchSymbols::visit(UsingDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(NamespaceAlias *)
{
    return false;
}

bool SearchSymbols::visit(Argument *)
{
    return false;
}

bool SearchSymbols::visit(TypenameArgument *)
{
    return false;
}

bool SearchSymbols::visit(BaseClass *)
{
    return false;
}

bool SearchSymbols::visit(Template *)
{
    return true;
}

bool SearchSymbols::visit(Block *)
{
    return false;
}

bool SearchSymbols::visit(ForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(ObjCBaseClass *)
{
    return false;
}

bool SearchSymbols::visit(ObjCBaseProtocol *)
{
    return false;
}

bool SearchSymbols::visit(ObjCClass *symbol)
{
    processClass(symbol);

    return false;
}

bool SearchSymbols::visit(ObjCForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(ObjCProtocol *symbol)
{
    processClass(symbol);

    return false;
}

bool SearchSymbols::visit(ObjCForwardProtocolDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(ObjCMethod *symbol)
{
    processFunction(symbol);
    return false;
}

bool SearchSymbols::visit(ObjCPropertyDeclaration *symbol)
{
    processFunction(symbol);
    return false;
}

QString SearchSymbols::scopedSymbolName(const QString &symbolName, const Symbol *symbol) const
{
    QString name = _scope;
    if (!name.isEmpty())
        name += QLatin1String("::");
    name += scopeName(symbolName, symbol);
    return name;
}

QString SearchSymbols::scopedSymbolName(const Symbol *symbol) const
{
    return scopedSymbolName(overview.prettyName(symbol->name()), symbol);
}

QString SearchSymbols::scopeName(const QString &name, const Symbol *symbol) const
{
    if (!name.isEmpty())
        return name;

    if (symbol->isNamespace()) {
        return QLatin1String("<anonymous namespace>");
    } else if (symbol->isEnum()) {
        return QLatin1String("<anonymous enum>");
    } else if (const Class *c = symbol->asClass())  {
        if (c->isUnion())
            return QLatin1String("<anonymous union>");
        else if (c->isStruct())
            return QLatin1String("<anonymous struct>");
        else
            return QLatin1String("<anonymous class>");
    } else {
        return QLatin1String("<anonymous symbol>");
    }
}

IndexItem::Ptr SearchSymbols::addChildItem(const QString &symbolName, const QString &symbolType,
                                           const QString &symbolScope, IndexItem::ItemType itemType,
                                           Symbol *symbol)
{
    if (!symbol->name() || symbol->isGenerated())
        return IndexItem::Ptr();

    QString path = m_paths.value(symbol->fileId(), QString());
    if (path.isEmpty()) {
        path = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
        m_paths.insert(symbol->fileId(), path);
    }

    const QIcon icon = Icons::iconForSymbol(symbol);
    IndexItem::Ptr newItem = IndexItem::create(findOrInsert(symbolName),
                                               findOrInsert(symbolType),
                                               findOrInsert(symbolScope),
                                               itemType,
                                               findOrInsert(path),
                                               symbol->line(),
                                               symbol->column() - 1, // 1-based vs 0-based column
                                               icon);
    _parent->addChild(newItem);
    return newItem;
}

template<class T>
void SearchSymbols::processClass(T *clazz)
{
    QString name = overview.prettyName(clazz->name());

    IndexItem::Ptr newParent;
    if (symbolsToSearchFor & SymbolSearcher::Classes)
        newParent = addChildItem(name, QString(), _scope, IndexItem::Class, clazz);
    if (!newParent)
        newParent = _parent;
    ScopedIndexItemPtr parentRaii(_parent, newParent);

    QString newScope = scopedSymbolName(name, clazz);
    ScopedScope scopeRaii(_scope, newScope);
    for (unsigned i = 0, ei = clazz->memberCount(); i != ei; ++i)
        accept(clazz->memberAt(i));
}

template<class T>
void SearchSymbols::processFunction(T *func)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Functions) || !func->name())
        return;
    QString name = overview.prettyName(func->name());
    QString type = overview.prettyType(func->type());
    addChildItem(name, type, _scope, IndexItem::Function, func);
}

} // namespace CppTools
