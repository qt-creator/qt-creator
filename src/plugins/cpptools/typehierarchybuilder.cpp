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
    explicit DerivedHierarchyVisitor(const QString &qualifiedName)
        : _qualifiedName(qualifiedName)
        , _unqualifiedName(unqualifyName(qualifiedName))
    {}

    void execute(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot);

    bool visit(CPlusPlus::Class *) override;

    const QList<CPlusPlus::Symbol *> &derived() { return _derived; }
    const QSet<QString> otherBases() { return _otherBases; }

private:
    CPlusPlus::LookupContext _context;
    QString _qualifiedName;
    QString _unqualifiedName;
    CPlusPlus::Overview _overview;
    QHash<CPlusPlus::Symbol *, QString> _actualBases;
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

bool DerivedHierarchyVisitor::visit(CPlusPlus::Class *symbol)
{
    for (int i = 0; i < symbol->baseClassCount(); ++i) {
        CPlusPlus::BaseClass *baseSymbol = symbol->baseClassAt(i);

        QString baseName = _actualBases.value(baseSymbol);
        if (baseName.isEmpty()) {
            QList<CPlusPlus::LookupItem> items = _context.lookup(baseSymbol->name(), symbol->enclosingScope());
            if (items.isEmpty() || !items.first().declaration())
                continue;

            CPlusPlus::Symbol *actualBaseSymbol = items.first().declaration();
            if (actualBaseSymbol->isTypedef()) {
                CPlusPlus::NamedType *namedType = actualBaseSymbol->type()->asNamedType();
                if (!namedType) {
                    // Anonymous aggregate such as: typedef struct {} Empty;
                    continue;
                }
                const QString &typeName = _overview.prettyName(namedType->name());
                if (typeName == _unqualifiedName || typeName == _qualifiedName) {
                    items = _context.lookup(namedType->name(), actualBaseSymbol->enclosingScope());
                    if (items.isEmpty() || !items.first().declaration())
                        continue;
                    actualBaseSymbol = items.first().declaration();
                }
            }

            const QList<const CPlusPlus::Name *> &full
                    = CPlusPlus::LookupContext::fullyQualifiedName(actualBaseSymbol);
            baseName = _overview.prettyName(full);
            _actualBases.insert(baseSymbol, baseName);
        }

        if (_qualifiedName == baseName)
            _derived.append(symbol);
        else
            _otherBases.insert(baseName);
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

TypeHierarchyBuilder::TypeHierarchyBuilder(CPlusPlus::Symbol *symbol, const CPlusPlus::Snapshot &snapshot)
    : _symbol(symbol)
    , _snapshot(snapshot)
{}

void TypeHierarchyBuilder::reset()
{
    _visited.clear();
    _candidates.clear();
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy()
{
    QFutureInterfaceBase dummy;
    return buildDerivedTypeHierarchy(dummy);
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy(QFutureInterfaceBase &futureInterface)
{
    reset();
    TypeHierarchy hierarchy(_symbol);
    buildDerived(futureInterface, &hierarchy, filesDependingOn(_symbol));
    return hierarchy;
}

void TypeHierarchyBuilder::buildDerived(QFutureInterfaceBase &futureInterface,
                                        TypeHierarchy *typeHierarchy,
                                        const QStringList &dependingFiles, int depth)
{
    CPlusPlus::Symbol *symbol = typeHierarchy->_symbol;
    if (_visited.contains(symbol))
        return;

    _visited.insert(symbol);

    const QString &symbolName = _overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));
    DerivedHierarchyVisitor visitor(symbolName);

    if (depth == 0)
        futureInterface.setProgressRange(0, dependingFiles.size());

    int i = -1;
    for (const QString &fileName : dependingFiles) {
        if (futureInterface.isCanceled())
            return;
        if (depth == 0)
            futureInterface.setProgressValue(++i);
        CPlusPlus::Document::Ptr doc = _snapshot.document(fileName);
        if ((_candidates.contains(fileName) && !_candidates.value(fileName).contains(symbolName))
                || !doc->control()->findIdentifier(symbol->identifier()->chars(),
                                                   symbol->identifier()->size())) {
            continue;
        }

        visitor.execute(doc, _snapshot);
        _candidates.insert(fileName, visitor.otherBases());

        const QList<CPlusPlus::Symbol *> &derived = visitor.derived();
        for (CPlusPlus::Symbol *s : derived) {
            TypeHierarchy derivedHierarchy(s);
            buildDerived(futureInterface, &derivedHierarchy, filesDependingOn(s), depth + 1);
            if (futureInterface.isCanceled())
                return;
            typeHierarchy->_hierarchy.append(derivedHierarchy);
        }
    }
}

QStringList TypeHierarchyBuilder::filesDependingOn(CPlusPlus::Symbol *symbol) const
{
    QStringList deps;
    if (!symbol)
        return deps;

    const Utils::FilePath file = Utils::FilePath::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    deps << file.toString();
    const Utils::FilePaths filePaths = _snapshot.filesDependingOn(file);
    for (const Utils::FilePath &fileName : filePaths)
        deps.append(fileName.toString());
    return deps;
}
