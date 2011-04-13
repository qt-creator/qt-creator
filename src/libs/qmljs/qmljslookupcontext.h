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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSLOOKUPCONTEXT_H
#define QMLJSLOOKUPCONTEXT_H

#include "qmljsdocument.h"
#include "qmljsinterpreter.h"
#include "parser/qmljsastfwd_p.h"

#include <QtCore/QSharedPointer>
#include <QtCore/QScopedPointer>

namespace QmlJS {

class LookupContextData;

namespace Interpreter {
class Value;
class Context;
}

class QMLJS_EXPORT LookupContext
{
    Q_DISABLE_COPY(LookupContext)

    LookupContext(const Document::Ptr doc, const Snapshot &snapshot, const QList<AST::Node *> &path);
    LookupContext(const Document::Ptr doc, const Snapshot &snapshot,
                  const Interpreter::Context &linkedContextWithoutScope,
                  const QList<AST::Node *> &path);

public:
    ~LookupContext();

    typedef QSharedPointer<LookupContext> Ptr;

    // consider using SemanticInfo::lookupContext instead, it's faster
    static Ptr create(const Document::Ptr doc, const Snapshot &snapshot,
                      const QList<AST::Node *> &path);
    static Ptr create(const Document::Ptr doc, const Snapshot &snapshot,
                      const Interpreter::Context &linkedContextWithoutScope,
                      const QList<AST::Node *> &path);

    const Interpreter::Value *evaluate(AST::Node *node) const;

    Document::Ptr document() const;
    Snapshot snapshot() const;
    Interpreter::Engine *engine() const;
    const Interpreter::Context *context() const;

private:
    QScopedPointer<LookupContextData> d;
};

} // namespace QmlJS

#endif // QMLJSLOOKUPCONTEXT_H

