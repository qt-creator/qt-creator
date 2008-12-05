/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "searchsymbols.h"

#include <Literals.h>
#include <Scope.h>

using namespace CPlusPlus;
using namespace CppTools::Internal;

SearchSymbols::SearchSymbols():
    symbolsToSearchFor(Classes | Functions | Enums)
{
}

void SearchSymbols::setSymbolsToSearchFor(SymbolTypes types)
{
    symbolsToSearchFor = types;
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
    QString previousScope = switchScope(name);
    QIcon icon = icons.iconForSymbol(symbol);
    Scope *members = symbol->members();
    items.append(ModelItemInfo(name, QString(), ModelItemInfo::Enum,
                               QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                               symbol->line(),
                               icon));
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

    QString name = symbolName(symbol);
    QString type = overview.prettyType(symbol->type());
    QIcon icon = icons.iconForSymbol(symbol);
    items.append(ModelItemInfo(name, type, ModelItemInfo::Method,
                               QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                               symbol->line(),
                               icon));
    return false;
}

bool SearchSymbols::visit(Namespace *symbol)
{
    QString name = symbolName(symbol);
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
        QString name = symbolName(symbol);
        QString type = overview.prettyType(symbol->type());
        //QIcon icon = ...;
        items.append(ModelItemInfo(name, type, ModelItemInfo::Method,
                                   QString::fromUtf8(symbol->fileName(), symbol->line()),
                                   symbol->line()));
    }
    return false;
}
#endif

bool SearchSymbols::visit(Class *symbol)
{
    if (!(symbolsToSearchFor & Classes))
        return false;

    QString name = symbolName(symbol);
    QString previousScope = switchScope(name);
    QIcon icon = icons.iconForSymbol(symbol);
    items.append(ModelItemInfo(name, QString(), ModelItemInfo::Class,
                               QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()),
                               symbol->line(),
                               icon));
    Scope *members = symbol->members();
    for (unsigned i = 0; i < members->symbolCount(); ++i) {
        accept(members->symbolAt(i));
    }
    (void) switchScope(previousScope);
    return false;
}

QString SearchSymbols::symbolName(const Symbol *symbol) const
{
    QString name = _scope;
    if (! name.isEmpty())
        name += QLatin1String("::");
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
    name += symbolName;
    return name;
}
