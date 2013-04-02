/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "typehierarchybuilder.h"

#include <cplusplus/DependencyTable.h>
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
    DerivedHierarchyVisitor(const QString &qualifiedName)
        : _qualifiedName(qualifiedName)
        , _unqualifiedName(unqualifyName(qualifiedName))
    {}

    void execute(const CPlusPlus::Document::Ptr &doc, const CPlusPlus::Snapshot &snapshot);

    virtual bool visit(CPlusPlus::Class *);

    const QList<CPlusPlus::Symbol *> &derived() { return _derived; }
    const QStringList otherBases() { return _otherBases; }

private:
    CPlusPlus::LookupContext _context;
    QString _qualifiedName;
    QString _unqualifiedName;
    CPlusPlus::Overview _overview;
    QHash<CPlusPlus::Symbol *, QString> _actualBases;
    QStringList _otherBases;
    QList<CPlusPlus::Symbol *> _derived;
};

void DerivedHierarchyVisitor::execute(const CPlusPlus::Document::Ptr &doc,
                                      const CPlusPlus::Snapshot &snapshot)
{
    _derived.clear();
    _otherBases.clear();
    _context = CPlusPlus::LookupContext(doc, snapshot);

    for (unsigned i = 0; i < doc->globalSymbolCount(); ++i)
        accept(doc->globalSymbolAt(i));
}

bool DerivedHierarchyVisitor::visit(CPlusPlus::Class *symbol)
{
    for (unsigned i = 0; i < symbol->baseClassCount(); ++i) {
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
            _otherBases.append(baseName);
    }

    return true;
}

} // namespace

TypeHierarchy::TypeHierarchy() : _symbol(0)
{}

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
    , _dependencies(QString::fromUtf8(symbol->fileName(), symbol->fileNameLength()))
{
    CPlusPlus::DependencyTable dependencyTable;
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
    CPlusPlus::Symbol *symbol = typeHierarchy->_symbol;
    if (_visited.contains(symbol))
        return;

    _visited.insert(symbol);

    const QString &symbolName = _overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(symbol));
    DerivedHierarchyVisitor visitor(symbolName);

    foreach (const QString &fileName, _dependencies) {
        CPlusPlus::Document::Ptr doc = _snapshot.document(fileName);
        if ((_candidates.contains(fileName) && !_candidates.value(fileName).contains(symbolName))
                || !doc->control()->findIdentifier(symbol->identifier()->chars(),
                                                   symbol->identifier()->size())) {
            continue;
        }

        visitor.execute(doc, _snapshot);
        _candidates.insert(fileName, QSet<QString>());

        foreach (const QString &candidate, visitor.otherBases())
            _candidates[fileName].insert(candidate);

        foreach (CPlusPlus::Symbol *s, visitor.derived()) {
            TypeHierarchy derivedHierarchy(s);
            buildDerived(&derivedHierarchy);
            typeHierarchy->_hierarchy.append(derivedHierarchy);
        }
    }
}
