/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    ScopeBuilder(Interpreter::Context *context, Document::Ptr doc, const Snapshot &snapshot);
    ~ScopeBuilder();

    void push(AST::Node *node);
    void push(const QList<AST::Node *> &nodes);
    void pop();

    static const Interpreter::ObjectValue *isPropertyChangesObject(const Interpreter::Context *context, const Interpreter::ObjectValue *object);

private:
    void initializeScopeChain();
    void makeComponentChain(Document::Ptr doc, Interpreter::ScopeChain::QmlComponentChain *target,
                            QHash<Document *, Interpreter::ScopeChain::QmlComponentChain *> *components);

    void setQmlScopeObject(AST::Node *node);
    const Interpreter::Value *scopeObjectLookup(AST::UiQualifiedId *id);

    Document::Ptr _doc;
    Snapshot _snapshot;
    Interpreter::Context *_context;
    QList<AST::Node *> _nodes;
};

} // namespace QmlJS

#endif // QMLJSSCOPEBUILDER_H
