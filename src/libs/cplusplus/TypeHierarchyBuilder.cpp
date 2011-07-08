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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "TypeHierarchyBuilder.h"
#include "FindUsages.h"
#include "Symbols.h"
#include "SymbolVisitor.h"
#include "DependencyTable.h"
#include "CppDocument.h"
#include "Literals.h"
#include "TranslationUnit.h"
#include "CoreTypes.h"

using namespace CPlusPlus;

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
    DerivedHierarchyVisitor(const QString &qualifiedName)
        : _qualifiedName(qualifiedName)
        , _unqualifiedName(unqualifyName(qualifiedName))
    {}

    void execute(const Document::Ptr &doc, const Snapshot &snapshot);

    virtual bool visit(Class *);

    const QList<Symbol *> &derived() { return _derived; }
    const QStringList otherBases() { return _otherBases; }

private:
    LookupContext _context;
    QString _qualifiedName;
    QString _unqualifiedName;
    Overview _overview;
    QHash<Symbol *, QString> _actualBases;
    QStringList _otherBases;
    QList<Symbol *> _derived;
};

void DerivedHierarchyVisitor::execute(const Document::Ptr &doc, const Snapshot &snapshot)
{
    _derived.clear();
    _otherBases.clear();
    _context = LookupContext(doc, snapshot);

    for (unsigned i = 0; i < doc->globalSymbolCount(); ++i)
        accept(doc->globalSymbolAt(i));
}

bool DerivedHierarchyVisitor::visit(Class *symbol)
{
    for (unsigned i = 0; i < symbol->baseClassCount(); ++i) {
        BaseClass *baseSymbol = symbol->baseClassAt(i);

        QString baseName = _actualBases.value(baseSymbol);
        if (baseName.isEmpty()) {
            QList<LookupItem> items = _context.lookup(baseSymbol->name(), symbol->enclosingScope());
            if (items.isEmpty() || !items.first().declaration())
                continue;

            Symbol *actualBaseSymbol = items.first().declaration();
            if (actualBaseSymbol->isTypedef()) {
                NamedType *namedType = actualBaseSymbol->type()->asNamedType();
                const QString &typeName = _overview.prettyName(namedType->name());
                if (namedType &&
                        (typeName == _unqualifiedName || typeName == _qualifiedName)) {
                    items = _context.lookup(namedType->name(), actualBaseSymbol->enclosingScope());
                    if (items.isEmpty() || !items.first().declaration())
                        continue;
                    actualBaseSymbol = items.first().declaration();
                }
            }

            const QList<const Name *> &full = LookupContext::fullyQualifiedName(actualBaseSymbol);
            baseName = _overview.prettyName(full);
            _actualBases.insert(baseSymbol, baseName);
        }

        if (_qualifiedName == baseName)
            _derived.append(symbol);
        else
            _otherBases.append(baseName);
    }

    return true;
}

}

TypeHierarchy::TypeHierarchy() : _symbol(0)
{}

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

TypeHierarchyBuilder::TypeHierarchyBuilder(Symbol *symbol, const Snapshot &snapshot)
    : _symbol(symbol)
    , _snapshot(snapshot)
    , _dependencies(QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()))
{
    DependencyTable dependencyTable;
    dependencyTable.build(_snapshot);
    _dependencies.append(dependencyTable.filesDependingOn(_dependencies.first()));
}

void TypeHierarchyBuilder::reset()
{
    _visited.clear();
    _candidates.clear();
}

TypeHierarchy TypeHierarchyBuilder::buildDerivedTypeHierarchy()
{
    reset();
    TypeHierarchy hierarchy(_symbol);
    buildDerived(&hierarchy);
    return hierarchy;
}

void TypeHierarchyBuilder::buildDerived(TypeHierarchy *typeHierarchy)
{
    Symbol *symbol = typeHierarchy->_symbol;
    if (_visited.contains(symbol))
        return;

    _visited.insert(symbol);

    const QString &symbolName = _overview.prettyName(LookupContext::fullyQualifiedName(symbol));
    DerivedHierarchyVisitor visitor(symbolName);

    foreach (const QString &fileName, _dependencies) {
        Document::Ptr doc = _snapshot.document(fileName);
        if ((_candidates.contains(fileName) && !_candidates.value(fileName).contains(symbolName))
                || !doc->control()->findIdentifier(symbol->identifier()->chars(),
                                                   symbol->identifier()->size())) {
            continue;
        }

        visitor.execute(doc, _snapshot);
        _candidates.insert(fileName, QSet<QString>());

        foreach (const QString &candidate, visitor.otherBases())
            _candidates[fileName].insert(candidate);

        foreach (Symbol *s, visitor.derived()) {
            TypeHierarchy derivedHierarchy(s);
            buildDerived(&derivedHierarchy);
            typeHierarchy->_hierarchy.append(derivedHierarchy);
        }
    }
}
