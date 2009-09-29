/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "LookupContext.h"
#include "ResolveExpression.h"
#include "Overview.h"

#include <CoreTypes.h>
#include <Symbols.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Control.h>

#include <QtDebug>

using namespace CPlusPlus;

/////////////////////////////////////////////////////////////////////
// LookupContext
/////////////////////////////////////////////////////////////////////
LookupContext::LookupContext(Control *control)
    : _control(control),
      _symbol(0)
{ }

LookupContext::LookupContext(Symbol *symbol,
                             Document::Ptr expressionDocument,
                             Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _symbol(symbol),
      _expressionDocument(expressionDocument),
      _thisDocument(thisDocument),
      _snapshot(snapshot)
{
    _control = _expressionDocument->control();
    _visibleScopes = buildVisibleScopes();
}

bool LookupContext::isValid() const
{ return _control != 0; }

Control *LookupContext::control() const
{ return _control; }

Symbol *LookupContext::symbol() const
{ return _symbol; }

Document::Ptr LookupContext::expressionDocument() const
{ return _expressionDocument; }

Document::Ptr LookupContext::thisDocument() const
{ return _thisDocument; }

Document::Ptr LookupContext::document(const QString &fileName) const
{ return _snapshot.value(fileName); }

Snapshot LookupContext::snapshot() const
{ return _snapshot; }

bool LookupContext::maybeValidSymbol(Symbol *symbol,
                                     ResolveMode mode,
                                     const QList<Symbol *> &candidates)
{
    if (((mode & ResolveNamespace) && symbol->isNamespace()) ||
        ((mode & ResolveClass)     && symbol->isClass())     ||
         (mode & ResolveSymbol)) {
        return ! candidates.contains(symbol);
    }

    return false;
}

QList<Scope *> LookupContext::resolveNestedNameSpecifier(QualifiedNameId *q,
                                                          const QList<Scope *> &visibleScopes) const
{
    QList<Symbol *> candidates;
    QList<Scope *> scopes = visibleScopes;

    for (unsigned i = 0; i < q->nameCount() - 1; ++i) {
        Name *name = q->nameAt(i);

        candidates = resolveClassOrNamespace(name, scopes);

        if (candidates.isEmpty())
            break;

        scopes.clear();

        foreach (Symbol *candidate, candidates) {
            ScopedSymbol *scoped = candidate->asScopedSymbol();
            Scope *members = scoped->members();

            if (! scopes.contains(members))
                scopes.append(members);
        }
    }

    return scopes;
}

QList<Symbol *> LookupContext::resolveQualifiedNameId(QualifiedNameId *q,
                                                      const QList<Scope *> &visibleScopes,
                                                      ResolveMode mode) const
{
    QList<Scope *> scopes;

    if (q->nameCount() == 1)
        scopes = visibleScopes;     // ### handle global scope lookup
    else
        scopes = resolveNestedNameSpecifier(q, visibleScopes);

    QList<Scope *> expanded;
    foreach (Scope *scope, scopes) {
        expanded.append(scope);

        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            Symbol *member = scope->symbolAt(i);

            if (ScopedSymbol *scopedSymbol = member->asScopedSymbol())
                expandEnumOrAnonymousSymbol(scopedSymbol, &expanded);
        }
    }

    return resolve(q->unqualifiedNameId(), expanded, mode);
}

QList<Symbol *> LookupContext::resolveOperatorNameId(OperatorNameId *opId,
                                                     const QList<Scope *> &visibleScopes,
                                                     ResolveMode) const
{
    QList<Symbol *> candidates;

    for (int scopeIndex = 0; scopeIndex < visibleScopes.size(); ++scopeIndex) {
        Scope *scope = visibleScopes.at(scopeIndex);

        for (Symbol *symbol = scope->lookat(opId->kind()); symbol; symbol = symbol->next()) {
            if (! opId->isEqualTo(symbol->name()))
                continue;

            if (! candidates.contains(symbol))
                candidates.append(symbol);
        }
    }

    return candidates;
}

QList<Symbol *> LookupContext::resolve(Name *name, const QList<Scope *> &visibleScopes,
                                       ResolveMode mode) const
{
    QList<Symbol *> candidates;

    if (!name)
        return candidates; // nothing to do, the symbol is anonymous.

    else if (QualifiedNameId *q = name->asQualifiedNameId())
        return resolveQualifiedNameId(q, visibleScopes, mode);

    else if (OperatorNameId *opId = name->asOperatorNameId())
        return resolveOperatorNameId(opId, visibleScopes, mode);

    else if (Identifier *id = name->identifier()) {
        for (int scopeIndex = 0; scopeIndex < visibleScopes.size(); ++scopeIndex) {
            Scope *scope = visibleScopes.at(scopeIndex);

            for (Symbol *symbol = scope->lookat(id); symbol; symbol = symbol->next()) {
                if (! symbol->name())
                    continue; // nothing to do, the symbol is anonymous.

                else if (! maybeValidSymbol(symbol, mode, candidates))
                    continue; // skip it, we're not looking for this kind of symbols


                else if (Identifier *symbolId = symbol->identifier()) {
                    if (! symbolId->isEqualTo(id))
                        continue; // skip it, the symbol's id is not compatible with this lookup.
                }


                if (QualifiedNameId *q = symbol->name()->asQualifiedNameId()) {

                    if (name->isDestructorNameId() != q->unqualifiedNameId()->isDestructorNameId())
                        continue;

                    else if (q->nameCount() > 1) {
                        Name *classOrNamespaceName = control()->qualifiedNameId(q->names(),
                                                                                q->nameCount() - 1);

                        if (Identifier *classOrNamespaceNameId = identifier(classOrNamespaceName)) {
                            if (classOrNamespaceNameId->isEqualTo(id))
                                continue;
                        }

                        const QList<Symbol *> resolvedClassOrNamespace =
                                resolveClassOrNamespace(classOrNamespaceName, visibleScopes);

                        bool good = false;
                        foreach (Symbol *classOrNamespace, resolvedClassOrNamespace) {
                            ScopedSymbol *scoped = classOrNamespace->asScopedSymbol();
                            if (visibleScopes.contains(scoped->members())) {
                                good = true;
                                break;
                            }
                        }

                        if (! good)
                            continue;
                    }
                } else if (symbol->name()->isDestructorNameId() != name->isDestructorNameId()) {
                    // ### FIXME: this is wrong!
                    continue;
                }

                if (! candidates.contains(symbol))
                    candidates.append(symbol);
            }
        }
    }

    return candidates;
}

Identifier *LookupContext::identifier(const Name *name) const
{
    if (name)
        return name->identifier();

    return 0;
}

void LookupContext::buildVisibleScopes_helper(Document::Ptr doc, QList<Scope *> *scopes,
                                              QSet<QString> *processed)
{
    if (doc && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        if (doc->globalSymbolCount())
            scopes->append(doc->globalSymbols());

        foreach (const Document::Include &incl, doc->includes()) {
            buildVisibleScopes_helper(_snapshot.value(incl.fileName()),
                                      scopes, processed);
        }
    }
}

QList<Scope *> LookupContext::buildVisibleScopes()
{
    QList<Scope *> scopes;

    if (_symbol) {
        Scope *scope = _symbol->scope();

        if (Function *fun = _symbol->asFunction())
            scope = fun->members(); // handle ctor initializers.

        for (; scope; scope = scope->enclosingScope()) {
            if (scope == _thisDocument->globalSymbols())
                break;

            scopes.append(scope);
        }
    }

    QSet<QString> processed;
    buildVisibleScopes_helper(_thisDocument, &scopes, &processed);

    while (true) {
        QList<Scope *> expandedScopes;
        expand(scopes, &expandedScopes);

        if (expandedScopes.size() == scopes.size())
            return expandedScopes;

        scopes = expandedScopes;
    }

    return scopes;
}

QList<Scope *> LookupContext::visibleScopes(const QPair<FullySpecifiedType, Symbol *> &result) const
{
    Symbol *symbol = result.second;
    QList<Scope *> scopes;
    for (Scope *scope = symbol->scope(); scope; scope = scope->enclosingScope())
        scopes.append(scope);
    scopes += visibleScopes();
    scopes = expand(scopes);
    return scopes;
}

void LookupContext::expandEnumOrAnonymousSymbol(ScopedSymbol *scopedSymbol,
                                                QList<Scope *> *expandedScopes) const
{
    if (! scopedSymbol || expandedScopes->contains(scopedSymbol->members()))
        return;

    Scope *members = scopedSymbol->members();

    if (scopedSymbol->isEnum())
        expandedScopes->append(members);
    else if (! scopedSymbol->name() && (scopedSymbol->isClass() || scopedSymbol->isNamespace())) {
        // anonymous class or namespace

        expandedScopes->append(members);

        for (unsigned i = 0; i < members->symbolCount(); ++i) {
            Symbol *member = members->symbolAt(i);

            if (ScopedSymbol *nested = member->asScopedSymbol()) {
                expandEnumOrAnonymousSymbol(nested, expandedScopes);
            }
        }
    }
}

QList<Scope *> LookupContext::expand(const QList<Scope *> &scopes) const
{
    QList<Scope *> expanded;
    expand(scopes, &expanded);
    return expanded;
}

void LookupContext::expand(const QList<Scope *> &scopes, QList<Scope *> *expandedScopes) const
{
    for (int i = 0; i < scopes.size(); ++i) {
        expand(scopes.at(i), scopes, expandedScopes);
    }
}

void LookupContext::expandNamespace(Namespace *ns,
                                    const QList<Scope *> &visibleScopes,
                                    QList<Scope *> *expandedScopes) const
{
    if (Name *nsName = ns->name()) {
        const QList<Symbol *> namespaceList = resolveNamespace(nsName, visibleScopes);
        foreach (Symbol *otherNs, namespaceList) {
            if (otherNs == ns)
                continue;
            expand(otherNs->asNamespace()->members(), visibleScopes, expandedScopes);
        }
    }

    for (unsigned i = 0; i < ns->memberCount(); ++i) { // ### make me fast
        Symbol *symbol = ns->memberAt(i);
        if (Namespace *otherNs = symbol->asNamespace()) {
            if (! otherNs->name()) {
                expand(otherNs->members(), visibleScopes, expandedScopes);
            }
        } else if (UsingNamespaceDirective *u = symbol->asUsingNamespaceDirective()) {
            const QList<Symbol *> candidates = resolveNamespace(u->name(), visibleScopes);
            for (int j = 0; j < candidates.size(); ++j) {
                expand(candidates.at(j)->asNamespace()->members(),
                       visibleScopes, expandedScopes);
            }
        } else if (Enum *e = symbol->asEnum()) {
            expand(e->members(), visibleScopes, expandedScopes);
        }
    }
}

void LookupContext::expandClass(Class *klass,
                                const QList<Scope *> &visibleScopes,
                                QList<Scope *> *expandedScopes) const
{
    for (unsigned i = 0; i < klass->memberCount(); ++i) {
        Symbol *symbol = klass->memberAt(i);
        if (Class *nestedClass = symbol->asClass()) {
            if (! nestedClass->name()) {
                expand(nestedClass->members(), visibleScopes, expandedScopes);
            }
        } else if (Enum *e = symbol->asEnum()) {
            expand(e->members(), visibleScopes, expandedScopes);
        }
    }

    if (klass->baseClassCount()) {
        QList<Scope *> classVisibleScopes = visibleScopes;
        for (Scope *scope = klass->scope(); scope; scope = scope->enclosingScope()) {
            if (scope->isNamespaceScope()) {
                Namespace *enclosingNamespace = scope->owner()->asNamespace();
                if (enclosingNamespace->name()) {
                    const QList<Symbol *> nsList = resolveNamespace(enclosingNamespace->name(),
                                                                    visibleScopes);
                    foreach (Symbol *ns, nsList) {
                        expand(ns->asNamespace()->members(), classVisibleScopes,
                               &classVisibleScopes);
                    }
                }
            }
        }

        for (unsigned i = 0; i < klass->baseClassCount(); ++i) {
            BaseClass *baseClass = klass->baseClassAt(i);
            Name *baseClassName = baseClass->name();
            const QList<Symbol *> baseClassCandidates = resolveClass(baseClassName,
                                                                     classVisibleScopes);

            for (int j = 0; j < baseClassCandidates.size(); ++j) {
                if (Class *baseClassSymbol = baseClassCandidates.at(j)->asClass())
                    expand(baseClassSymbol->members(), visibleScopes, expandedScopes);
            }
        }
    }
}

void LookupContext::expandBlock(Block *blockSymbol,
                                const QList<Scope *> &visibleScopes,
                                QList<Scope *> *expandedScopes) const
{
    for (unsigned i = 0; i < blockSymbol->memberCount(); ++i) {
        Symbol *symbol = blockSymbol->memberAt(i);
        if (UsingNamespaceDirective *u = symbol->asUsingNamespaceDirective()) {
            const QList<Symbol *> candidates = resolveNamespace(u->name(),
                                                                visibleScopes);
            for (int j = 0; j < candidates.size(); ++j) {
                expand(candidates.at(j)->asNamespace()->members(),
                       visibleScopes, expandedScopes);
            }
        }

    }
}

void LookupContext::expandFunction(Function *function,
                                   const QList<Scope *> &visibleScopes,
                                   QList<Scope *> *expandedScopes) const
{
    if (! expandedScopes->contains(function->arguments()))
        expandedScopes->append(function->arguments());

    if (QualifiedNameId *q = function->name()->asQualifiedNameId()) {
        Name *nestedNameSpec = 0;
        if (q->nameCount() == 1)
            nestedNameSpec = q->nameAt(0);
        else
            nestedNameSpec = control()->qualifiedNameId(q->names(), q->nameCount() - 1,
                                                        q->isGlobal());
        const QList<Symbol *> candidates = resolveClassOrNamespace(nestedNameSpec, visibleScopes);
        for (int j = 0; j < candidates.size(); ++j) {
            expand(candidates.at(j)->asScopedSymbol()->members(),
                   visibleScopes, expandedScopes);
        }
    }
}

void LookupContext::expandObjCMethod(ObjCMethod *method,
                                     const QList<Scope *> &,
                                     QList<Scope *> *expandedScopes) const
{
    if (! expandedScopes->contains(method->arguments()))
        expandedScopes->append(method->arguments());
}

void LookupContext::expand(Scope *scope,
                           const QList<Scope *> &visibleScopes,
                           QList<Scope *> *expandedScopes) const
{
    if (expandedScopes->contains(scope))
        return;

    expandedScopes->append(scope);

    if (Namespace *ns = scope->owner()->asNamespace()) {
        expandNamespace(ns, visibleScopes, expandedScopes);
    } else if (Class *klass = scope->owner()->asClass()) {
        expandClass(klass, visibleScopes, expandedScopes);
    } else if (Block *block = scope->owner()->asBlock()) {
        expandBlock(block, visibleScopes, expandedScopes);
    } else if (Function *fun = scope->owner()->asFunction()) {
        expandFunction(fun, visibleScopes, expandedScopes);
    } else if (ObjCMethod *meth = scope->owner()->asObjCMethod()) {
        expandObjCMethod(meth, visibleScopes, expandedScopes);
    }
}

Symbol *LookupContext::canonicalSymbol(Symbol *symbol)
{
    Symbol *canonical = symbol;
    for (; symbol; symbol = symbol->next()) {
        if (symbol->name() == canonical->name())
            canonical = symbol;
    }
    return canonical;
}

Symbol *LookupContext::canonicalSymbol(const QList<Symbol *> &candidates)
{
    if (candidates.isEmpty())
        return 0;

    Symbol *candidate = candidates.first();
    if (candidate->scope()->isClassScope() && candidate->type()->isFunctionType()) {
        Function *function = 0;

        for (int i = 0; i < candidates.size(); ++i) {
            Symbol *c = candidates.at(i);

            if (! c->scope()->isClassScope())
                continue; 

            else if (Function *f = c->type()->asFunctionType()) {
                if (f->isVirtual())
                    function = f;
            }
        }

        if (function)
            return canonicalSymbol(function);
    }

    return canonicalSymbol(candidate);
}

Symbol *LookupContext::canonicalSymbol(const QList<QPair<FullySpecifiedType, Symbol *> > &results)
{
    QList<Symbol *> candidates;
    QPair<FullySpecifiedType, Symbol *> result;
    foreach (result, results) {
        candidates.append(result.second); // ### not exacly.
    }
    return canonicalSymbol(candidates);
}
