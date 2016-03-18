/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "qmljs_global.h"
#include "qmljsdocument.h"
#include "qmljsinterpreter.h"
#include "qmljsviewercontext.h"

#include <QSharedPointer>

namespace QmlJS {

class Document;
class Snapshot;
class Context;
typedef QSharedPointer<const Context> ContextPtr;

// shared among threads, completely threadsafe
class QMLJS_EXPORT Context
{
    Q_DISABLE_COPY(Context)
public:
    typedef QHash<const Document *, QSharedPointer<const Imports> > ImportsPerDocument;

    // Context takes ownership of valueOwner
    static ContextPtr create(const Snapshot &snapshot, ValueOwner *valueOwner,
                             const ImportsPerDocument &imports, const ViewerContext &viewerContext);
    ~Context();

    ContextPtr ptr() const;

    ValueOwner *valueOwner() const;
    Snapshot snapshot() const;
    ViewerContext viewerContext() const;

    const Imports *imports(const Document *doc) const;

    const ObjectValue *lookupType(const Document *doc, AST::UiQualifiedId *qmlTypeName,
                                  AST::UiQualifiedId *qmlTypeNameEnd = 0) const;
    const ObjectValue *lookupType(const Document *doc, const QStringList &qmlTypeName) const;
    const Value *lookupReference(const Value *value) const;

    QString defaultPropertyName(const ObjectValue *object) const;

private:
    // Context takes ownership of valueOwner
    Context(const Snapshot &snapshot, ValueOwner *valueOwner, const ImportsPerDocument &imports,
            const ViewerContext &viewerContext);

    Snapshot _snapshot;
    QSharedPointer<ValueOwner> _valueOwner;
    ImportsPerDocument _imports;
    ViewerContext _vContext;
    QWeakPointer<const Context> _ptr;
};

// for looking up references
class QMLJS_EXPORT ReferenceContext
{
public:
    // implicit conversion ok
    ReferenceContext(const ContextPtr &context);

    const Value *lookupReference(const Value *value);

    const ContextPtr &context() const;
    operator const ContextPtr &() const;

private:
    const ContextPtr &m_context;
    QList<const Reference *> m_references;
};

} // namespace QmlJS
