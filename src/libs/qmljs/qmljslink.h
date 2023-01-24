// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljscontext.h>

#include <QCoreApplication>

namespace QmlJS {

class NameId;
class LinkPrivate;

/*
    Helper for building a context.
*/
class QMLJS_EXPORT Link
{
    Q_DISABLE_COPY(Link)

public:
    Link(Link &&) = delete;
    Link &operator=(Link &&) = delete;

    Link(const Snapshot &snapshot, const ViewerContext &vContext, const LibraryInfo &builtins);

    // Link all documents in snapshot, collecting all diagnostic messages (if messages != 0)
    ContextPtr operator()(QHash<Utils::FilePath, QList<DiagnosticMessage>> *messages = nullptr);

    // Link all documents in snapshot, appending the diagnostic messages
    // for 'doc' in 'messages'
    ContextPtr operator()(const Document::Ptr &doc, QList<DiagnosticMessage> *messages);

    ~Link();

private:
    LinkPrivate *d;
    friend class LinkPrivate;
};

} // namespace QmlJS
