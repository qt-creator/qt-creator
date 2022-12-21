// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
                                  AST::UiQualifiedId *qmlTypeNameEnd = nullptr) const;
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
    const ContextPtr m_context;
    QList<const Reference *> m_references;
};

} // namespace QmlJS
