/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "searchsymbols.h"

#include <Literals.h>
#include <Scope.h>
#include <Names.h>

using namespace CPlusPlus;
using namespace CppTools::Internal;

SearchSymbols::SearchSymbols():
    symbolsToSearchFor(Classes | Functions | Enums),
    separateScope(false)
{
}

void SearchSymbols::setSymbolsToSearchFor(SymbolTypes types)
{
    symbolsToSearchFor = types;
}

void SearchSymbols::setSeparateScope(bool separateScope)
{
    this->separateScope = separateScope;
}

QList<ModelItemInfo> SearchSymbols::operator()(Document::Ptr doc, const QString &scope)
{
    QString previousScope = switchScope(scope);
    items.clear();
    for (unsigned i = 0; i < doc->globalSymbolCount(); ++i) {
        accept(doc->globalSymbolAt(i));
    }
    (void) switchScope(previousScope);
    return items;
}

QString SearchSymbols::switchScope(const QString &scope)
{
    QString previousScope = _scope;
    _scope = scope;
    return previousScope;
}

bool SearchSymbols::visit(Enum *symbol)
{
    if (!(symbolsToSearchFor & Enums))
        return false;

    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString previousScope = switchScope(scopedName);
    appendItem(separateScope ? name : scopedName,
               separateScope ? previousScope : QString(),
               ModelItemInfo::Enum, symbol);
    Scope *members = symbol->members();
    for (unsigned i = 0; i < members->symbolCount(); ++i) {
        accept(members->symbolAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

bool SearchSymbols::visit(Function *symbol)
{
    if (!(symbolsToSearchFor & Functions))
        return false;

    QString extraScope;
    if (Name *name = symbol->name()) {
        if (QualifiedNameId *nameId = name->asQualifiedNameId()) {
            if (nameId->nameCount() > 1) {
                extraScope = overview.prettyName(nameId->nameAt(nameId->nameCount() - 2));
            }
        }
    }
    QString fullScope = _scope;
    if (!_scope.isEmpty() && !extraScope.isEmpty())
        fullScope += QLatin1String("::");
    fullScope += extraScope;
    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString type = overview.prettyType(symbol->type(),
                                       separateScope ? symbol->identity() : 0);
    appendItem(separateScope ? type : scopedName,
               separateScope ? fullScope : type,
               ModelItemInfo::Method, symbol);
    return false;
}

bool SearchSymbols::visit(Namespace *symbol)
{
    QString name = findOrInsert(scopedSymbolName(symbol));
    QString previousScope = switchScope(name);
    Scope *members = symbol->members();
    for (unsigned i = 0; i < members->symbolCount(); ++i) {
        accept(members->symbolAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

#if 0
bool SearchSymbols::visit(Declaration *symbol)
{
    if (symbol->type()->isFunction()) {
        QString name = scopedSymbolName(symbol);
        QString type = overview.prettyType(symbol->type());
        appendItems(name, type, ModelItemInfo::Method, symbol->fileName());
    }
    return false;
}
#endif

bool SearchSymbols::visit(Class *symbol)
{
    if (!(symbolsToSearchFor & Classes))
        return false;

    QString name = symbolName(symbol);
    QString scopedName = scopedSymbolName(name);
    QString previousScope = switchScope(scopedName);
    appendItem(separateScope ? name : scopedName,
               separateScope ? previousScope : QString(),
               ModelItemInfo::Class, symbol);
    Scope *members = symbol->members();
    for (unsigned i = 0; i < members->symbolCount(); ++i) {
        accept(members->symbolAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

QString SearchSymbols::scopedSymbolName(const QString &symbolName) const
{
    QString name = _scope;
    if (!name.isEmpty())
        name += QLatin1String("::");
    name += symbolName;
    return name;
}

QString SearchSymbols::scopedSymbolName(const Symbol *symbol) const
{
    return scopedSymbolName(symbolName(symbol));
}

QString SearchSymbols::symbolName(const Symbol *symbol) const
{
    QString symbolName = overview.prettyName(symbol->name());
    if (symbolName.isEmpty()) {
        QString type;
        if (symbol->isNamespace()) {
            type = QLatin1String("namespace");
        } else if (symbol->isEnum()) {
            type = QLatin1String("enum");
        } else if (const Class *c = symbol->asClass())  {
            if (c->isUnion()) {
                type = QLatin1String("union");
            } else if (c->isStruct()) {
                type = QLatin1String("struct");
            } else {
                type = QLatin1String("class");
            }
        } else {
            type = QLatin1String("symbol");
        }
        symbolName = QLatin1String("<anonymous ");
        symbolName += type;
        symbolName += QLatin1String(">");
    }
    return symbolName;
}

void SearchSymbols::appendItem(const QString &name,
                               const QString &info,
                               ModelItemInfo::ItemType type,
                               const Symbol *symbol)
{
    if (!symbol->name())
        return;

    const QIcon icon = icons.iconForSymbol(symbol);
    items.append(ModelItemInfo(name, info, type,
                               QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                               symbol->line(),
                               icon));
}
