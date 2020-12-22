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

#include "typehierarchybuilder.h"

#include <cplusplus/FindUsages.h>

using namespace CppTools;

namespace {

QString unqualifyName(const QString &qualifiedName)
{
    const int index = qualifiedName.lastIndexOf(QLatin1String("::"));
    if (index == -1)
        return qualifiedName;
    return qualifiedName.right(qualifiedName.length() - index - 2);
}

class DerivedHierarchyVisitor : public CPlusPlus::SymbolVisitor
{
public:
    explicit DerivedHierarchyVisitor(const QString &qualifiedName, QHash<QString, QHash<QString, QString>> &cache)
        : _qualifiedName(qualifiedName)
        , _unqualifiedName(unqualifyName(qualifiedName))
        , _cache(cache)
    {}

    void execute(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot);

    bool visit(CPlusPlus::Class *) override;

    const QList<CPlusPlus::Symbol *> &derived() { return _derived; }
    const QSet<QString> otherBases() { return _otherBases; }

private:
    CPlusPlus::Symbol *lookup(const CPlusPlus::Name *symbolName, CPlusPlus::Scope *enclosingScope);

    CPlusPlus::LookupContext _context;
    QString _qualifiedName;
    QString _unqualifiedName;
    CPlusPlus::Overview _overview;
    // full scope name to base symbol name to fully qualified base symbol name
    QHash<QString, QHash<QString, QString>> &_cache;
    QSet<QString> _otherBases;
    QList<CPlusPlus::Symbol *> _derived;
};

void DerivedHierarchyVisitor::execute(const CPlusPlus::Document::Ptr &doc,
                                      const CPlusPlus::Snapshot &snapshot)
{
    _derived.clear();
    _otherBases.clear();
    _context = CPlusPlus::LookupContext(doc, snapshot);

    for (int i = 0; i < doc->globalSymbolCount(); ++i)
        accept(doc->globalSymbolAt(i));
}

CPlusPlus::Symbol *DerivedHierarchyVisitor::lookup(const CPlusPlus::Name *symbolName, CPlusPlus::Scope *enclosingScope)
{
    QList<CPlusPlus::LookupItem> items = _context.lookup(symbolName, enclosingScope);

    CPlusPlus::Symbol *actualBaseSymbol = nullptr;

    for (const CPlusPlus::LookupItem &item : items) {
        CPlusPlus::Symbol *s = item.declaration();
        if (!s)
            continue;
        if (!s->isClass() && !s->isTemplate() && !s->isTypedef())
            continue;
        actualBaseSymbol = s;
        break;
    }

    if (!actualBaseSymbol)
        return nullptr;

    if (actualBaseSymbol->isTypedef()) {
        CPlusPlus::NamedType *namedType = actualBaseSymbol->type()->asNamedType();
        if (!namedType) {
            // Anonymous aggregate such as: typedef struct {} Empty;
            return nullptr;
        }
        actualBaseSymbol = lookup(namedType->name(), actualBaseSymbol->enclosingScope());
    }

    return actualBaseSymbol;
}

bool DerivedHierarchyVisitor::visit(CPlusPlus::Class *symbol)
{
    const QList<const CPlusPlus::Name *> &fullScope
            = CPlusPlus::LookupContext::fullyQualifiedName(symbol->enclosingScope());
    const QString fullScopeName = _overview.prettyName(fullScope);

    for (int i = 0; i < symbol->baseClassCount(); ++i) {
        CPlusPlus::BaseClass *baseSymbol = symbol->baseClassAt(i);

        const QString &baseName = _overview.prettyName(baseSymbol->name());
        QString fullBaseName = _cache.value(fullScopeName).value(baseName);
        if (fullBaseName.isEmpty()) {
            CPlusPlus::Symbol *actualBaseSymbol = lookup(baseSymbol->name(), symbol->enclosingScope());
            if (!actualBaseSymbol)
                continue;

            const QList<const CPlusPlus::Name *> &full
                    = CPlusPlus::LookupContext::fullyQualifiedName(actualBaseSymbol);
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

TypeHierarchy::TypeHierarchy(CPlusPlus::Symbol *symbol) : _symbol(symbol)
{}

CPlusPlus::Symbol *TypeHierarchy::symbol() const
{
    return _symbol;
}

const QList<TypeHierarchy> &TypeHierarchy::hierarchy() const
{
    return _hierarchy;
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy(CPlusPlus::Symbol *symbol,
                                                              const CPlusPlus::Snapshot &snapshot)
{
    QFutureInterfaceBase dummy;
    return TypeHierarchyBuilder::buildDerivedTypeHierarchy(dummy, symbol, snapshot);
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy(QFutureInterfaceBase &futureInterface,
                                                              CPlusPlus::Symbol *symbol,
                                                              const CPlusPlus::Snapshot &snapshot)
{
    TypeHierarchy hierarchy(symbol);
    TypeHierarchyBuilder builder;
    QHash<QString, QHash<QString, QString>> cache;
    builder.buildDerived(futureInterface, &hierarchy, snapshot, cache);
    return hierarchy;
}

static Utils::FilePaths filesDependingOn(const CPlusPlus::Snapshot &snapshot,
                                         CPlusPlus::Symbol *symbol)
{
    if (!symbol)
        return Utils::FilePaths();

    const Utils::FilePath file = Utils::FilePath::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    return Utils::FilePaths { file } + snapshot.filesDependingOn(file);
}

void TypeHierarchyBuilder::buildDerived(QFutureInterfaceBase &futureInterface,
                                        TypeHierarchy *typeHierarchy,
                                        const CPlusPlus::Snapshot &snapshot,
                                        QHash<QString, QHash<QString, QString>> &cache,
                                        int depth)
{
    CPlusPlus::Symbol *symbol = typeHierarchy->_symbol;
    if (_visited.contains(symbol))
        return;

    _visited.insert(symbol);

    const QString &symbolName = _overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));
    DerivedHierarchyVisitor visitor(symbolName, cache);

    const Utils::FilePaths &dependingFiles = filesDependingOn(snapshot, symbol);
    if (depth == 0)
        futureInterface.setProgressRange(0, dependingFiles.size());

    int i = -1;
    for (const Utils::FilePath &fileName : dependingFiles) {
        if (futureInterface.isCanceled())
            return;
        if (depth == 0)
            futureInterface.setProgressValue(++i);
        CPlusPlus::Document::Ptr doc = snapshot.document(fileName);
        if ((_candidates.contains(fileName) && !_candidates.value(fileName).contains(symbolName))
                || !doc->control()->findIdentifier(symbol->identifier()->chars(),
                                                   symbol->identifier()->size())) {
            continue;
        }

        visitor.execute(doc, snapshot);
        _candidates.insert(fileName, visitor.otherBases());

        const QList<CPlusPlus::Symbol *> &derived = visitor.derived();
        for (CPlusPlus::Symbol *s : derived) {
            TypeHierarchy derivedHierarchy(s);
            buildDerived(futureInterface, &derivedHierarchy, snapshot, cache, depth + 1);
            if (futureInterface.isCanceled())
                return;
            typeHierarchy->_hierarchy.append(derivedHierarchy);
        }
    }
}
