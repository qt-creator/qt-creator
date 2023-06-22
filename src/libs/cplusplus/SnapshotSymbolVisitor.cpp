// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "SnapshotSymbolVisitor.h"

#include <cplusplus/Symbols.h>

#include <utils/algorithm.h>

using namespace CPlusPlus;

SnapshotSymbolVisitor::SnapshotSymbolVisitor(const Snapshot &snapshot)
    : _snapshot(snapshot)
{
}

void SnapshotSymbolVisitor::accept(Document::Ptr doc)
{
    QSet<QString> processed;
    accept(doc, &processed);
}

void SnapshotSymbolVisitor::accept(Document::Ptr doc, QSet<QString> *processed)
{
    if (doc && doc->globalNamespace() && Utils::insert(*processed, doc->filePath().path())) {
        const QList<Document::Include> includes = doc->resolvedIncludes();
        for (const Document::Include &i : includes) {
            if (Document::Ptr incl = _snapshot.document(i.resolvedFileName()))
                accept(incl, processed);
        }

        std::swap(_document, doc);
        accept(_document->globalNamespace());
        std::swap(_document, doc);
    }
}
