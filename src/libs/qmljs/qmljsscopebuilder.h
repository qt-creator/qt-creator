/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLJSSCOPEBUILDER_H
#define QMLJSSCOPEBUILDER_H

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>

#include <QtCore/QList>

namespace QmlJS {

namespace AST {
    class Node;
}

class QMLJS_EXPORT ScopeBuilder
{
public:
    ScopeBuilder(Interpreter::Context *context, Document::Ptr doc);
    ~ScopeBuilder();

    void initializeRootScope();

    void push(AST::Node *node);
    void push(const QList<AST::Node *> &nodes);
    void pop();

    static const Interpreter::ObjectValue *isPropertyChangesObject(const Interpreter::Context *context, const Interpreter::ObjectValue *object);

private:
    void makeComponentChain(Document::Ptr doc, const Snapshot &snapshot,
                            Interpreter::ScopeChain::QmlComponentChain *target,
                            QHash<Document *, Interpreter::ScopeChain::QmlComponentChain *> *components);

    void setQmlScopeObject(AST::Node *node);
    const Interpreter::Value *scopeObjectLookup(AST::UiQualifiedId *id);

    Document::Ptr _doc;
    Interpreter::Context *_context;
    QList<AST::Node *> _nodes;
};

} // namespace QmlJS

#endif // QMLJSSCOPEBUILDER_H
