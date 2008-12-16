/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "LookupContext.h"
#include <CoreTypes.h>
#include <Symbols.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Control.h>
#include <cplusplus/Overview.h>

#include <QFile>
#include <QtDebug>

using namespace CPlusPlus;

/////////////////////////////////////////////////////////////////////
// LookupUtils
/////////////////////////////////////////////////////////////////////
bool LookupUtils::isNameCompatibleWithIdentifier(Name *name, Identifier *id)
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

QList<Symbol *> LookupContext::resolve(Name *name, const QList<Scope *> &visibleScopes,
                                       ResolveMode mode) const
{
    if (QualifiedNameId *q = name->asQualifiedNameId()) {
        QList<Scope *> scopes = visibleScopes;
        for (unsigned i = 0; i < q->nameCount(); ++i) {
            Name *name = q->nameAt(i);

            QList<Symbol *> candidates;
            if (i + 1 == q->nameCount())
                candidates = resolve(name, scopes, mode);
            else
                candidates = resolveClassOrNamespace(name, scopes);

            if (candidates.isEmpty() || i + 1 == q->nameCount())
                return candidates;

            scopes.clear();
            foreach (Symbol *candidate, candidates) {
                if (ScopedSymbol *scoped = candidate->asScopedSymbol()) {
                    expand(scoped->members(), visibleScopes, &scopes);
                }
            }
        }

        return QList<Symbol *>();
    }

    QList<Symbol *> candidates;
    if (Identifier *id = identifier(name)) {
        for (int scopeIndex = 0; scopeIndex < visibleScopes.size(); ++scopeIndex) {
            Scope *scope = visibleScopes.at(scopeIndex);
            for (Symbol *symbol = scope->lookat(id); symbol; symbol = symbol->next()) {
                if (! symbol->name())
                    continue;
                else if (symbol->name()->isQualifiedNameId())
                    continue;
                else if (! isNameCompatibleWithIdentifier(symbol->name(), id))
                    continue;
                else if (symbol->name()->isDestructorNameId() != name->isDestructorNameId())
                    continue;
                else if ((((mode & ResolveNamespace) && symbol->isNamespace()) ||
                          ((mode & ResolveClass) && symbol->isClass())         ||
                          (mode & ResolveSymbol)) && ! candidates.contains(symbol)) {
                    candidates.append(symbol);
                }
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

QList<Scope *> LookupContext::buildVisibleScopes()
{
    QList<Scope *> scopes;

    if (_symbol) {
        for (Scope *scope = _symbol->scope(); scope; scope = scope->enclosingScope()) {
            scopes.append(scope);
        }
    }

    QSet<QString> processed;
    processed.insert(_thisDocument->fileName());

    QList<QString> todo = _thisDocument->includedFiles();
    while (! todo.isEmpty()) {
        QString fn = todo.last();
        todo.removeLast();

        if (processed.contains(fn))
            continue;

        processed.insert(fn);
        if (Document::Ptr doc = document(fn)) {
            scopes.append(doc->globalNamespace()->members());
            todo += doc->includedFiles();
        }
    }

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

void LookupContext::expand(Scope *scope,
                           const QList<Scope *> &visibleScopes,
                           QList<Scope *> *expandedScopes) const
{
    Overview overview;

    if (expandedScopes->contains(scope)) {
        //qDebug() << "skipped:" << overview.prettyName(scope->owner()->name());
        return;
    }

    expandedScopes->append(scope);

    if (scope->isNamespaceScope()) {
        Namespace *ns = scope->owner()->asNamespace();
        Name *nsName = ns->name();
        if (nsName) {
            QList<Symbol *> namespaceList = resolveNamespace(nsName, visibleScopes);
            foreach (Symbol *otherNs, namespaceList) {
                if (otherNs == ns)
                    continue;
                expand(otherNs->asNamespace()->members(), visibleScopes, expandedScopes);
            }
            //qDebug() << "*** found:" << namespaceList.count() << "namespace aliases";
        }
        //qDebug() << "namespace scope" << overview.prettyName(ns->name())
                //<< ns->fileName() << ns->line();
        for (unsigned i = 0; i < scope->symbolCount(); ++i) { // ### make me fast
            Symbol *symbol = scope->symbolAt(i);
            if (Namespace *ns = symbol->asNamespace()) {
                if (! ns->name()) {
                    expand(ns->members(), visibleScopes, expandedScopes);
                }
            } else if (UsingNamespaceDirective *u = symbol->asUsingNamespaceDirective()) {
                QList<Symbol *> candidates = resolveNamespace(u->name(), visibleScopes);
                //qDebug() << "found:" << candidates.count() << "namespaces to import for:"
                        //<< overview.prettyName(u->name());
                for (int j = 0; j < candidates.size(); ++j) {
                    expand(candidates.at(j)->asNamespace()->members(),
                           visibleScopes, expandedScopes);
                }
            } else if (Enum *e = symbol->asEnum()) {
                expand(e->members(), visibleScopes, expandedScopes);
            }
        }
    } else if (scope->isClassScope()) {
        Class *klass = scope->owner()->asClass();
        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            Symbol *symbol = scope->symbolAt(i);
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
                        QList<Symbol *> nsList = resolveNamespace(enclosingNamespace->name(),
                                                                  visibleScopes);
                        foreach (Symbol *ns, nsList) {
                            expand(ns->asNamespace()->members(), classVisibleScopes, &classVisibleScopes);
                        }
                    }
                }
            }

            for (unsigned i = 0; i < klass->baseClassCount(); ++i) {
                BaseClass *baseClass = klass->baseClassAt(i);
                Name *baseClassName = baseClass->name();
                QList<Symbol *> baseClassCandidates = resolveClass(baseClassName, classVisibleScopes);
                if (baseClassCandidates.isEmpty()) {
                    Overview overview;
                    qDebug() << "unresolved base class:" << overview.prettyName(baseClassName);
                }
                for (int j = 0; j < baseClassCandidates.size(); ++j) {
                    Class *baseClassSymbol = baseClassCandidates.at(j)->asClass();
                    expand(baseClassSymbol->members(), visibleScopes, expandedScopes);
                }
            }
        }
    } else if (scope->isBlockScope()) {
        //qDebug() << "block scope" << overview.prettyName(scope->owner()->name());
        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            Symbol *symbol = scope->symbolAt(i);
            if (UsingNamespaceDirective *u = symbol->asUsingNamespaceDirective()) {
                QList<Symbol *> candidates = resolveNamespace(u->name(), visibleScopes);
                //qDebug() << "found:" << candidates.count() << "namespaces to import for:"
                        //<< overview.prettyName(u->name());
                for (int j = 0; j < candidates.size(); ++j) {
                    expand(candidates.at(j)->asNamespace()->members(),
                           visibleScopes, expandedScopes);
                }
            }

        }
    } else if (scope->isFunctionScope()) {
        Function *function = scope->owner()->asFunction();
        //qDebug() << "function scope" << overview.prettyName(function->name());
        if (! expandedScopes->contains(function->arguments()))
            expandedScopes->append(function->arguments());
        if (QualifiedNameId *q = function->name()->asQualifiedNameId()) {
            //qDebug() << "**** here:" << overview.prettyName(function->name());
            Name *nestedNameSpec = 0;
            if (q->nameCount() == 1 && q->isGlobal())
                nestedNameSpec = q->nameAt(0);
            else
                nestedNameSpec = control()->qualifiedNameId(q->names(), q->nameCount() - 1,
                                                            q->isGlobal());
            QList<Symbol *> candidates = resolveClassOrNamespace(nestedNameSpec, visibleScopes);
            //qDebug() << "**** found:" << candidates.count() << "class or namespace for:"
                    //<< overview.prettyName(nestedNameSpec);
            for (int j = 0; j < candidates.size(); ++j) {
                expand(candidates.at(j)->asScopedSymbol()->members(),
                       visibleScopes, expandedScopes);
            }
        }
    } else if (scope->isPrototypeScope()) {
        //qDebug() << "prototype scope" << overview.prettyName(scope->owner()->name());
    }
}
