/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qmljslookupcontext.h"
#include "qmljsinterpreter.h"
#include "qmljslink.h"
#include "qmljsscopebuilder.h"
#include "qmljsmodelmanagerinterface.h"
#include "qmljsevaluate.h"

using namespace QmlJS;

class QmlJS::LookupContextData
{
public:
    LookupContextData(Document::Ptr doc, const Snapshot &snapshot, const QList<AST::Node *> &path)
        : doc(doc),
          snapshot(snapshot)
    {
        // since we keep the document and snapshot around, we don't need to keep the Link instance
        Link link(&context, doc, snapshot, ModelManagerInterface::instance()->importPaths());

        ScopeBuilder scopeBuilder(&context, doc, snapshot);
        scopeBuilder.push(path);
    }

    LookupContextData(Document::Ptr doc, const Snapshot &snapshot,
                      const Interpreter::Context &linkedContextWithoutScope,
                      const QList<AST::Node *> &path)
        : context(linkedContextWithoutScope),
          doc(doc),
          snapshot(snapshot)
    {
        ScopeBuilder scopeBuilder(&context, doc, snapshot);
        scopeBuilder.push(path);
    }

    Interpreter::Context context;
    Document::Ptr doc;
    Snapshot snapshot;
};

LookupContext::LookupContext(Document::Ptr doc, const Snapshot &snapshot, const QList<AST::Node *> &path)
    : d(new LookupContextData(doc, snapshot, path))
{
}

LookupContext::LookupContext(const Document::Ptr doc, const Snapshot &snapshot,
                             const Interpreter::Context &linkedContextWithoutScope,
                             const QList<AST::Node *> &path)
    : d(new LookupContextData(doc, snapshot, linkedContextWithoutScope, path))
{
}

LookupContext::~LookupContext()
{
}

LookupContext::Ptr LookupContext::create(Document::Ptr doc, const Snapshot &snapshot, const QList<AST::Node *> &path)
{
    Ptr ptr(new LookupContext(doc, snapshot, path));
    return ptr;
}

LookupContext::Ptr LookupContext::create(const Document::Ptr doc, const Snapshot &snapshot,
                                         const Interpreter::Context &linkedContextWithoutScope,
                                         const QList<AST::Node *> &path)
{
    Ptr ptr(new LookupContext(doc, snapshot, linkedContextWithoutScope, path));
    return ptr;
}

const Interpreter::Value *LookupContext::evaluate(AST::Node *node) const
{
    Evaluate check(&d->context);
    return check(node);
}

Document::Ptr LookupContext::document() const
{
    return d->doc;
}

Snapshot LookupContext::snapshot() const
{
    return d->snapshot;
}

// the engine is only guaranteed to live as long as the LookupContext
Interpreter::Engine *LookupContext::engine() const
{
    return d->context.engine();
}

// the context is only guaranteed to live as long as the LookupContext
const Interpreter::Context *LookupContext::context() const
{
    return &d->context;
}
