/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "searchsymbols.h"

#include <cplusplus/LookupContext.h>

#include <QDebug>

using namespace CPlusPlus;
using namespace CppTools;

SearchSymbols::SymbolTypes SearchSymbols::AllTypes =
        SymbolSearcher::Classes
        | SymbolSearcher::Functions
        | SymbolSearcher::Enums
        | SymbolSearcher::Declarations;

SearchSymbols::SearchSymbols() :
    symbolsToSearchFor(SymbolSearcher::Classes | SymbolSearcher::Functions | SymbolSearcher::Enums)
{
}

void SearchSymbols::setSymbolsToSearchFor(const SymbolTypes &types)
{
    symbolsToSearchFor = types;
}

QList<ModelItemInfo> SearchSymbols::operator()(Document::Ptr doc, int sizeHint, const QString &scope)
{
    QString previousScope = switchScope(scope);
    items.clear();
    items.reserve(sizeHint);
    for (unsigned i = 0; i < doc->globalSymbolCount(); ++i) {
        accept(doc->globalSymbolAt(i));
    }
    (void) switchScope(previousScope);
    QList<ModelItemInfo> result = items;
    strings.clear();
    items.clear();
    m_paths.clear();
    return result;
}

QString SearchSymbols::switchScope(const QString &scope)
{
    QString previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool SearchSymbols::visit(Enum *symbol)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Enums))
        return false;

    QString name = overview.prettyName(symbol->name());
    QString scopedName = scopedSymbolName(name, symbol);
    QString previousScope = switchScope(scopedName);
    appendItem(name, QString(), previousScope, ModelItemInfo::Enum, symbol);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(Function *symbol)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Functions) || !symbol->name())
        return false;
    QString name = overview.prettyName(symbol->name());
    QString type = overview.prettyType(symbol->type());
    appendItem(name, type, _scope, ModelItemInfo::Function, symbol);
    return false;
}

bool SearchSymbols::visit(Namespace *symbol)
{
    QString name = scopedSymbolName(symbol);
    QString previousScope = switchScope(name);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(Declaration *symbol)
{
    if (!(symbolsToSearchFor & SymbolSearcher::Declarations)) {
        // if we're searching for functions, still allow signal declarations to show up.
        if (symbolsToSearchFor & SymbolSearcher::Functions) {
            Function *funTy = symbol->type()->asFunctionType();
            if (!funTy)
                return false;
            if (!funTy->isSignal())
                return false;
        } else {
            return false;
        }
    }

    if (symbol->name()) {
        QString name = overview.prettyName(symbol->name());
        QString type = overview.prettyType(symbol->type());
        appendItem(name, type, _scope,
                   symbol->type()->asFunctionType() ? ModelItemInfo::Function
                                                    : ModelItemInfo::Declaration,
                   symbol);
    }

    return false;
}

bool SearchSymbols::visit(Class *symbol)
{
    QString name = overview.prettyName(symbol->name());
    QString scopedName = scopedSymbolName(name, symbol);
    QString previousScope = switchScope(scopedName);
    if (symbolsToSearchFor & SymbolSearcher::Classes)
        appendItem(name, QString(), previousScope, ModelItemInfo::Class, symbol);
    for (unsigned i = 0; i < symbol->memberCount(); ++i) {
        accept(symbol->memberAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(CPlusPlus::UsingNamespaceDirective *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::UsingDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::NamespaceAlias *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::Argument *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::TypenameArgument *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::BaseClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::Template *)
{
    return true;
}

bool SearchSymbols::visit(CPlusPlus::Block *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCBaseClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCBaseProtocol *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCClass *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCForwardClassDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCProtocol *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCForwardProtocolDeclaration *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCMethod *)
{
    return false;
}

bool SearchSymbols::visit(CPlusPlus::ObjCPropertyDeclaration *)
{
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

void SearchSymbols::appendItem(const QString &symbolName, const QString &symbolType,
                               const QString &symbolScope, ModelItemInfo::ItemType itemType,
                               Symbol *symbol)
{
    if (!symbol->name() || symbol->isGenerated())
        return;

    QString path = m_paths.value(symbol->fileId(), QString());
    if (path.isEmpty()) {
        path = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
        m_paths.insert(symbol->fileId(), path);
    }

    const QIcon icon = icons.iconForSymbol(symbol);
    items.append(ModelItemInfo(findOrInsert(symbolName),
                               findOrInsert(symbolType),
                               findOrInsert(symbolScope),
                               itemType,
                               path,
                               symbol->line(),
                               symbol->column() - 1, // 1-based vs 0-based column
                               icon));
}
