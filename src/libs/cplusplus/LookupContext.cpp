/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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

bool LookupContext::isNameCompatibleWithIdentifier(Name *name, Identifier *id)
{
    if (! name) {
        return false;
    } else if (NameId *nameId = name->asNameId()) {
        Identifier *identifier = nameId->identifier();
        return identifier->isEqualTo(id);
    } else if (DestructorNameId *nameId = name->asDestructorNameId()) {
        Identifier *identifier = nameId->identifier();
        return identifier->isEqualTo(id);
    } else if (TemplateNameId *templNameId = name->asTemplateNameId()) {
        Identifier *identifier = templNameId->identifier();
        return identifier->isEqualTo(id);
    }

    return false;
}

#ifndef CPLUSPLUS_WITH_NO_DEBUG
static void printScopes(const QList<Scope *> &scopes)
{
    qDebug() << "===========";
    foreach (Scope *scope, scopes) {
        qDebug() << "scope:" << scope << scope->owner()->name() << scope->owner()->fileName()
                << scope->owner()->line() << scope->owner()->column();
    }
}
#endif

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
                             const Snapshot &documents)
    : _symbol(symbol),
      _expressionDocument(expressionDocument),
      _thisDocument(thisDocument),
      _documents(documents)
{
    _control = _expressionDocument->control();
    _visibleScopes = buildVisibleScopes();
}

LookupContext::LookupContext(Symbol *symbol,
                             const LookupContext &context)
    : _control(context._control),
      _symbol(symbol),
      _expressionDocument(context._expressionDocument),
      _documents(context._documents)
{
    const QString fn = QString::fromUtf8(symbol->fileName(), symbol->fileNameLength());
    _thisDocument = _documents.value(fn);
    _visibleScopes = buildVisibleScopes();
}

LookupContext::LookupContext(Symbol *symbol,
              Document::Ptr thisDocument,
              const LookupContext &context)
    : _control(context._control),
      _symbol(symbol),
      _expressionDocument(context._expressionDocument),
      _thisDocument(thisDocument),
      _documents(context._documents)
{
    _visibleScopes = buildVisibleScopes();
}

bool LookupContext::isValid() const
{ return _control != 0; }

LookupContext::operator bool() const
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
{ return _documents.value(fileName); }

Identifier *LookupContext::identifier(Name *name) const
{
    if (NameId *nameId = name->asNameId())
        return nameId->identifier();
    else if (TemplateNameId *templId = name->asTemplateNameId())
        return templId->identifier();
    else if (DestructorNameId *dtorId = name->asDestructorNameId())
        return dtorId->identifier();
    else if (QualifiedNameId *q = name->asQualifiedNameId())
        return identifier(q->unqualifiedNameId());
    return 0;
}

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

QList<Symbol *> LookupContext::resolve(Name *name, const QList<Scope *> &visibleScopes,
                                       ResolveMode mode) const
{
    QList<Symbol *> candidates;

    if (!name)
        return candidates;

    if (QualifiedNameId *q = name->asQualifiedNameId()) {
        QList<Scope *> scopes = visibleScopes;
        for (unsigned i = 0; i < q->nameCount(); ++i) {
            Name *name = q->nameAt(i);

            if (i + 1 == q->nameCount())
                candidates = resolve(name, scopes, mode);
            else
                candidates = resolveClassOrNamespace(name, scopes);

            if (candidates.isEmpty() || i + 1 == q->nameCount())
                break;

            scopes.clear();
            foreach (Symbol *candidate, candidates) {
                if (ScopedSymbol *scoped = candidate->asScopedSymbol()) {
                    scopes.append(scoped->members());
                }
            }
        }

        Identifier *id = identifier(name);
        foreach (Scope *scope, visibleScopes) {
            Symbol *symbol = scope->lookat(id);
            for (; symbol; symbol = symbol->next()) {
                if (! symbol->name())
                    continue;
                else if (! maybeValidSymbol(symbol, mode, candidates))
                    continue;
                QualifiedNameId *qq = symbol->name()->asQualifiedNameId();
                if (! qq)
                    continue;
                if (q->nameCount() > qq->nameCount())
                    continue;

                for (int i = q->nameCount() - 1; i != -1; --i) {
                    Name *a = q->nameAt(i);
                    Name *b = qq->nameAt(i);

                    if (! a->isEqualTo(b))
                        break;
                    else if (i == 0)
                        candidates.append(symbol);
                }
            }
        }

        return candidates;
    }

    if (Identifier *id = identifier(name)) {
        for (int scopeIndex = 0; scopeIndex < visibleScopes.size(); ++scopeIndex) {
            Scope *scope = visibleScopes.at(scopeIndex);
            for (Symbol *symbol = scope->lookat(id); symbol; symbol = symbol->next()) {
                if (! symbol->name()) {
                    continue;
                } else if (! maybeValidSymbol(symbol, mode, candidates)) {
                    continue;
                } else if (QualifiedNameId *q = symbol->name()->asQualifiedNameId()) {
                    if (! q->unqualifiedNameId()->isEqualTo(name))
                        continue;

                    if (q->nameCount() > 1) {
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
                } else if (! isNameCompatibleWithIdentifier(symbol->name(), id)) {
                    continue;
                } else if (symbol->name()->isDestructorNameId() != name->isDestructorNameId()) {
                    continue;
                }

                candidates.append(symbol);
            }
        }
    } else if (OperatorNameId *opId = name->asOperatorNameId()) {
        for (int scopeIndex = 0; scopeIndex < visibleScopes.size(); ++scopeIndex) {
            Scope *scope = visibleScopes.at(scopeIndex);
            for (Symbol *symbol = scope->lookat(opId->kind()); symbol; symbol = symbol->next()) {
                if (! opId->isEqualTo(symbol->name()))
                    continue;
                else if (! candidates.contains(symbol))
                    candidates.append(symbol);
            }
        }
    }

    return candidates;
}

void LookupContext::buildVisibleScopes_helper(Document::Ptr doc, QList<Scope *> *scopes,
                                              QSet<QString> *processed)
{
    if (doc && ! processed->contains(doc->fileName())) {
        processed->insert(doc->fileName());

        if (doc->globalSymbolCount())
            scopes->append(doc->globalSymbols());

        foreach (const Document::Include &incl, doc->includes()) {
            buildVisibleScopes_helper(_documents.value(incl.fileName()),
                                      scopes, processed);
        }
    }
}

QList<Scope *> LookupContext::buildVisibleScopes()
{
    QList<Scope *> scopes;

    if (_symbol) {
        for (Scope *scope = _symbol->scope(); scope; scope = scope->enclosingScope()) {
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
                expand(ns->members(), visibleScopes, expandedScopes);
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

#if 0
            if (baseClassCandidates.isEmpty()) {
                Overview overview;
                qDebug() << "unresolved base class:" << overview.prettyName(baseClassName);
            }
#endif

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
    }
}
