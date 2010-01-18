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

#include <QDebug>

#include <qmljs/parser/qmljsast_p.h>

#include "qmlidcollector.h"

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace Qml;
using namespace Qml::Internal;

QMap<QString, QmlIdSymbol*> QmlIdCollector::operator()(Qml::QmlDocument &doc)
{
    _doc = &doc;
    _ids.clear();
    _currentSymbol = 0;

    Node::accept(doc.qmlProgram(), this);

    return _ids;
}

bool QmlIdCollector::visit(UiArrayBinding *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->members, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiObjectBinding *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->initializer, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    QmlSymbolFromFile *oldSymbol = switchSymbol(ast);
    Node::accept(ast->initializer, this);
    _currentSymbol = oldSymbol;
    return false;
}

bool QmlIdCollector::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (!(ast->qualifiedId->next) && ast->qualifiedId->name->asString() == "id")
        if (ExpressionStatement *e = cast<ExpressionStatement*>(ast->statement))
            if (IdentifierExpression *i = cast<IdentifierExpression*>(e->expression))
                if (i->name)
                    addId(i->name->asString(), ast);

    return false;
}

QmlSymbolFromFile *QmlIdCollector::switchSymbol(QmlJS::AST::UiObjectMember *node)
{
    QmlSymbolFromFile *newSymbol = 0;

    if (_currentSymbol == 0) {
        newSymbol = _doc->findSymbol(node);
    } else {
        newSymbol = _currentSymbol->findMember(node);
    }

    QmlSymbolFromFile *oldSymbol = _currentSymbol;

    if (newSymbol) {
        _currentSymbol = newSymbol;
    } else {
        QString filename = _doc->fileName();
        qWarning() << "Scope without symbol @"<<filename<<":"<<node->firstSourceLocation().startLine<<":"<<node->firstSourceLocation().startColumn;
    }

    return oldSymbol;
}

void QmlIdCollector::addId(const QString &id, QmlJS::AST::UiScriptBinding *ast)
{
    if (!_currentSymbol)
        return;

    if (_ids.contains(id)) {
        _diagnosticMessages.append(DiagnosticMessage(DiagnosticMessage::Warning, ast->statement->firstSourceLocation(), "Duplicate ID"));
    } else {
        if (QmlSymbolFromFile *symbol = _currentSymbol->findMember(ast))
            if (QmlIdSymbol *idSymbol = symbol->asIdSymbol())
                _ids[id] = idSymbol;
    }
}
