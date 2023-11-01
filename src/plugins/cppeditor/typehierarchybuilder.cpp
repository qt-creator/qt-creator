// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "typehierarchybuilder.h"

#include <cplusplus/FindUsages.h>

#include <utils/algorithm.h>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

QString unqualifyName(const QString &qualifiedName)
{
    const int index = qualifiedName.lastIndexOf(QLatin1String("::"));
    if (index == -1)
        return qualifiedName;
    return qualifiedName.right(qualifiedName.length() - index - 2);
}

class DerivedHierarchyVisitor : public SymbolVisitor
{
public:
    explicit DerivedHierarchyVisitor(const QString &qualifiedName, QHash<QString, QHash<QString, QString>> &cache)
        : _qualifiedName(qualifiedName)
        , _unqualifiedName(unqualifyName(qualifiedName))
        , _cache(cache)
    {}

    void execute(const Document::Ptr &doc, const Snapshot &snapshot);

    bool visit(Class *) override;

    const QList<Symbol *> &derived() { return _derived; }
    const QSet<QString> otherBases() { return _otherBases; }

private:
    Symbol *lookup(const Name *symbolName, Scope *enclosingScope);

    LookupContext _context;
    QString _qualifiedName;
    QString _unqualifiedName;
    Overview _overview;
    // full scope name to base symbol name to fully qualified base symbol name
    QHash<QString, QHash<QString, QString>> &_cache;
    QSet<QString> _otherBases;
    QList<Symbol *> _derived;
};

void DerivedHierarchyVisitor::execute(const Document::Ptr &doc,
                                      const Snapshot &snapshot)
{
    _derived.clear();
    _otherBases.clear();
    _context = LookupContext(doc, snapshot);

    for (int i = 0; i < doc->globalSymbolCount(); ++i)
        accept(doc->globalSymbolAt(i));
}

bool DerivedHierarchyVisitor::visit(Class *symbol)
{
    const QList<const Name *> &fullScope
            = LookupContext::fullyQualifiedName(symbol->enclosingScope());
    const QString fullScopeName = _overview.prettyName(fullScope);

    for (int i = 0; i < symbol->baseClassCount(); ++i) {
        BaseClass *baseSymbol = symbol->baseClassAt(i);

        const QString &baseName = _overview.prettyName(baseSymbol->name());
        QString fullBaseName = _cache.value(fullScopeName).value(baseName);
        if (fullBaseName.isEmpty()) {
            Symbol *actualBaseSymbol = TypeHierarchyBuilder::followTypedef(_context,
                                       baseSymbol->name(), symbol->enclosingScope()).declaration();
            if (!actualBaseSymbol)
                continue;

            const QList<const Name *> &full
                    = LookupContext::fullyQualifiedName(actualBaseSymbol);
            fullBaseName = _overview.prettyName(full);
            _cache[fullScopeName].insert(baseName, fullBaseName);
        }

        if (_qualifiedName == fullBaseName)
            _derived.append(symbol);
        else
            _otherBases.insert(fullBaseName);
    }
    return true;
}

} // namespace

TypeHierarchy::TypeHierarchy() = default;

TypeHierarchy::TypeHierarchy(Symbol *symbol) : _symbol(symbol)
{}

Symbol *TypeHierarchy::symbol() const
{
    return _symbol;
}

const QList<TypeHierarchy> &TypeHierarchy::hierarchy() const
{
    return _hierarchy;
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy(Symbol *symbol,
              const Snapshot &snapshot, const std::optional<QFuture<void>> &future)
{
    TypeHierarchy hierarchy(symbol);
    TypeHierarchyBuilder builder;
    QHash<QString, QHash<QString, QString>> cache;
    builder.buildDerived(future, &hierarchy, snapshot, cache);
    return hierarchy;
}

LookupItem TypeHierarchyBuilder::followTypedef(const LookupContext &context, const Name *symbolName,
                                               Scope *enclosingScope,
                                               std::set<const Symbol *> typedefs)
{
    QList<LookupItem> items = context.lookup(symbolName, enclosingScope);

    Symbol *actualBaseSymbol = nullptr;
    LookupItem matchingItem;

    for (const LookupItem &item : items) {
        Symbol *s = item.declaration();
        if (!s)
            continue;
        if (!s->asClass() && !s->asTemplate() && !s->isTypedef())
            continue;
        if (!typedefs.insert(s).second)
            continue;
        actualBaseSymbol = s;
        matchingItem = item;
        break;
    }

    if (!actualBaseSymbol)
        return LookupItem();

    if (actualBaseSymbol->isTypedef()) {
        NamedType *namedType = actualBaseSymbol->type()->asNamedType();
        if (!namedType) {
            // Anonymous aggregate such as: typedef struct {} Empty;
            return LookupItem();
        }
        return followTypedef(context, namedType->name(), actualBaseSymbol->enclosingScope(),
                             typedefs);
    }

    return matchingItem;
}

static FilePaths filesDependingOn(const Snapshot &snapshot, Symbol *symbol)
{
    if (!symbol)
        return {};

    const FilePath file = symbol->filePath();
    return FilePaths{file} + snapshot.filesDependingOn(file);
}

void TypeHierarchyBuilder::buildDerived(const std::optional<QFuture<void>> &future,
                                        TypeHierarchy *typeHierarchy,
                                        const Snapshot &snapshot,
                                        QHash<QString, QHash<QString, QString>> &cache)
{
    Symbol *symbol = typeHierarchy->_symbol;
    if (!Utils::insert(_visited, symbol))
        return;

    const QString &symbolName = _overview.prettyName(LookupContext::fullyQualifiedName(symbol));
    DerivedHierarchyVisitor visitor(symbolName, cache);

    const FilePaths dependingFiles = filesDependingOn(snapshot, symbol);

    for (const FilePath &fileName : dependingFiles) {
        if (future && future->isCanceled())
            return;
        Document::Ptr doc = snapshot.document(fileName);
        if ((_candidates.contains(fileName) && !_candidates.value(fileName).contains(symbolName))
                || !doc->control()->findIdentifier(symbol->identifier()->chars(),
                                                   symbol->identifier()->size())) {
            continue;
        }

        visitor.execute(doc, snapshot);
        _candidates.insert(fileName, visitor.otherBases());

        const QList<Symbol *> &derived = visitor.derived();
        for (Symbol *s : derived) {
            TypeHierarchy derivedHierarchy(s);
            buildDerived(future, &derivedHierarchy, snapshot, cache);
            if (future && future->isCanceled())
                return;
            typeHierarchy->_hierarchy.append(derivedHierarchy);
        }
    }
}

} // CppEditor::Internal
