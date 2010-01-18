/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmlresolveexpression.h"

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlResolveExpression::QmlResolveExpression(const QmlLookupContext &context)
    : _context(context), _value(0)
{
}

Symbol *QmlResolveExpression::typeOf(Node *node)
{
    Symbol *previousValue = switchValue(0);
    Node::accept(node, this);
    return switchValue(previousValue);
}

QList<Symbol*> QmlResolveExpression::visibleSymbols(Node *node)
{
    QList<Symbol*> result;

    Symbol *symbol = typeOf(node);
    if (symbol) {
        if (symbol->isIdSymbol())
            symbol = symbol->asIdSymbol()->parentNode();
        result.append(symbol->members());

        // TODO: also add symbols from super-types
    } else {
        result.append(_context.visibleTypes());
    }

    if (node) {
        foreach (IdSymbol *idSymbol, _context.document()->ids().values())
            result.append(idSymbol);
    }

    result.append(_context.visibleSymbolsInScope());

    return result;
}

Symbol *QmlResolveExpression::switchValue(Symbol *value)
{
    Symbol *previousValue = _value;
    _value = value;
    return previousValue;
}

bool QmlResolveExpression::visit(IdentifierExpression *ast)
{
    const QString name = ast->name->asString();
    _value = _context.resolve(name);
    return false;
}

static inline bool matches(UiQualifiedId *candidate, const QString &wanted)
{
    if (!candidate)
        return false;

    if (!(candidate->name))
        return false;

    if (candidate->next)
        return false; // TODO: verify this!

    return wanted == candidate->name->asString();
}

bool QmlResolveExpression::visit(FieldMemberExpression *ast)
{
    const QString memberName = ast->name->asString();

    Symbol *base = typeOf(ast->base);
    if (!base)
        return false;

    if (base->isIdSymbol())
        base = base->asIdSymbol()->parentNode();

    if (!base)
        return false;

    foreach (Symbol *memberSymbol, base->members())
        if (memberSymbol->name() == memberName)
            _value = memberSymbol;

    return false;
}

bool QmlResolveExpression::visit(QmlJS::AST::UiQualifiedId *ast)
{
    _value = _context.resolveType(ast);

    return false;
}
