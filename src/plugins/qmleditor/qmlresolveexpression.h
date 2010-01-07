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

#ifndef QMLRESOLVEEXPRESSION_H
#define QMLRESOLVEEXPRESSION_H

#include "qmljsastvisitor_p.h"
#include "qmllookupcontext.h"
#include "qmlsymbol.h"

namespace QmlEditor {
namespace Internal {

class QmlResolveExpression: protected QmlJS::AST::Visitor
{
public:
    QmlResolveExpression(const QmlLookupContext &context);
    ~QmlResolveExpression();

    QmlSymbol *operator()(QmlJS::AST::Node *node)
    { return typeOf(node); }

protected:
    using QmlJS::AST::Visitor::visit;

    QmlSymbol *typeOf(QmlJS::AST::Node *node);
    QmlSymbol *switchValue(QmlSymbol *symbol);

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);

private:
    QmlPropertyDefinitionSymbol *createPropertyDefinitionSymbol(QmlJS::AST::UiPublicMember *ast);
    QmlSymbolFromFile *createSymbolFromFile(QmlJS::AST::UiObjectMember *ast);

private:
    QmlLookupContext _context;
    QList<QmlSymbol*> _temporarySymbols;
    QmlSymbol *_value;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLRESOLVEEXPRESSION_H
