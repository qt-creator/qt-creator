// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "CppDocument.h"

#include <cplusplus/SymbolVisitor.h>

#include <QSet>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT SnapshotSymbolVisitor : public CPlusPlus::SymbolVisitor
{
public:
    SnapshotSymbolVisitor(const Snapshot &snapshot);

    void accept(Document::Ptr doc);
    using SymbolVisitor::accept;

protected:
    void accept(Document::Ptr doc, QSet<QString> *processed);

private:
    Snapshot _snapshot;
    Document::Ptr _document;
};

} // namespace CPlusPlus
