// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CppDocument.h>
#include <cplusplus/Overview.h>

#include <QFuture>
#include <QList>
#include <QSet>

#include <set>

namespace CPlusPlus {
class LookupContext;
class LookupItem;
class Name;
class Scope;
}

namespace CppEditor::Internal {

class TypeHierarchy
{
    friend class TypeHierarchyBuilder;

public:
    TypeHierarchy();
    explicit TypeHierarchy(CPlusPlus::Symbol *symbol);

    CPlusPlus::Symbol *symbol() const;
    const QList<TypeHierarchy> &hierarchy() const;

    bool operator==(const TypeHierarchy &other) const
    { return _symbol == other._symbol; }

private:
    CPlusPlus::Symbol *_symbol = nullptr;
    QList<TypeHierarchy> _hierarchy;
};

class TypeHierarchyBuilder
{
public:
    static TypeHierarchy buildDerivedTypeHierarchy(CPlusPlus::Symbol *symbol,
                                                   const CPlusPlus::Snapshot &snapshot,
                                                   const std::optional<QFuture<void>> &future = {});
    static CPlusPlus::LookupItem followTypedef(const CPlusPlus::LookupContext &context,
                                               const CPlusPlus::Name *symbolName,
                                               CPlusPlus::Scope *enclosingScope,
                                               std::set<const CPlusPlus::Symbol *> typedefs = {});
private:
    TypeHierarchyBuilder() = default;
    void buildDerived(const std::optional<QFuture<void>> &future, TypeHierarchy *typeHierarchy,
                      const CPlusPlus::Snapshot &snapshot,
                      QHash<QString, QHash<QString, QString> > &cache);

    QSet<CPlusPlus::Symbol *> _visited;
    QHash<Utils::FilePath, QSet<QString> > _candidates;
    CPlusPlus::Overview _overview;
};

} // CppEditor::Internal
