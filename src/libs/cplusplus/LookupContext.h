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

#ifndef CPLUSPLUS_LOOKUPCONTEXT_H
#define CPLUSPLUS_LOOKUPCONTEXT_H

#include "CppDocument.h"
#include <QPair>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext(Control *control = 0);

    LookupContext(Symbol *symbol,
                  Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    bool isValid() const;

    Control *control() const;
    Symbol *symbol() const;
    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    static Symbol *canonicalSymbol(const QList<Symbol *> &candidates,
                                   NamespaceBinding *globalNamespaceBinding);

    static Symbol *canonicalSymbol(Symbol *symbol,
                                   NamespaceBinding *globalNamespaceBinding);

    static Symbol *canonicalSymbol(const QList<QPair<FullySpecifiedType, Symbol *> > &candidates,
                                   NamespaceBinding *globalNamespaceBinding);

    QList<Symbol *> resolve(Name *name) const
    { return resolve(name, visibleScopes()); }

    QList<Symbol *> resolveNamespace(Name *name) const
    { return resolveNamespace(name, visibleScopes()); }

    QList<Symbol *> resolveClass(Name *name) const
    { return resolveClass(name, visibleScopes()); }

    QList<Symbol *> resolveClassOrNamespace(Name *name) const
    { return resolveClassOrNamespace(name, visibleScopes()); }

    enum ResolveMode {
        ResolveSymbol           = 0x01,
        ResolveClass            = 0x02,
        ResolveNamespace        = 0x04,
        ResolveClassOrNamespace = ResolveClass  | ResolveNamespace,
        ResolveAll              = ResolveSymbol | ResolveClassOrNamespace
    };

    QList<Symbol *> resolve(Name *name, const QList<Scope *> &visibleScopes,
                            ResolveMode mode = ResolveAll) const;

    QList<Symbol *> resolveNamespace(Name *name, const QList<Scope *> &visibleScopes) const
    { return resolve(name, visibleScopes, ResolveNamespace); }

    QList<Symbol *> resolveClass(Name *name, const QList<Scope *> &visibleScopes) const
    { return resolve(name, visibleScopes, ResolveClass); }

    QList<Symbol *> resolveClassOrNamespace(Name *name, const QList<Scope *> &visibleScopes) const
    { return resolve(name, visibleScopes, ResolveClassOrNamespace); }

    QList<Scope *> visibleScopes() const
    { return _visibleScopes; }

    QList<Scope *> visibleScopes(const QPair<FullySpecifiedType, Symbol *> &result) const;

    QList<Scope *> expand(const QList<Scope *> &scopes) const;

    void expand(const QList<Scope *> &scopes, QList<Scope *> *expandedScopes) const;

    void expand(Scope *scope, const QList<Scope *> &visibleScopes,
                QList<Scope *> *expandedScopes) const;

    void expandNamespace(Namespace *namespaceSymbol,
                         const QList<Scope *> &visibleScopes,
                         QList<Scope *> *expandedScopes) const;

    void expandClass(Class *classSymbol,
                     const QList<Scope *> &visibleScopes,
                     QList<Scope *> *expandedScopes) const;

    void expandBlock(Block *blockSymbol,
                     const QList<Scope *> &visibleScopes,
                     QList<Scope *> *expandedScopes) const;

    void expandFunction(Function *functionSymbol,
                        const QList<Scope *> &visibleScopes,
                        QList<Scope *> *expandedScopes) const;

    void expandObjCMethod(ObjCMethod *method,
                          const QList<Scope *> &visibleScopes,
                          QList<Scope *> *expandedScopes) const;

    void expandEnumOrAnonymousSymbol(ScopedSymbol *scopedSymbol,
                                     QList<Scope *> *expandedScopes) const;

private:
    static Symbol *canonicalSymbol(Symbol *symbol);

    QList<Symbol *> resolveQualifiedNameId(QualifiedNameId *q,
                                           const QList<Scope *> &visibleScopes,
                                           ResolveMode mode) const;

    QList<Symbol *> resolveOperatorNameId(OperatorNameId *opId,
                                          const QList<Scope *> &visibleScopes,
                                          ResolveMode mode) const;

    QList<Scope *> resolveNestedNameSpecifier(QualifiedNameId *q,
                                               const QList<Scope *> &visibleScopes) const;

    Identifier *identifier(const Name *name) const;

    QList<Scope *> buildVisibleScopes();

    void buildVisibleScopes_helper(Document::Ptr doc, QList<Scope *> *scopes,
                                   QSet<QString> *processed);

    static bool maybeValidSymbol(Symbol *symbol,
                                 ResolveMode mode,
                                 const QList<Symbol *> &candidates);

private:
    Control *_control;

    // The current symbol.
    Symbol *_symbol;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Visible scopes.
    QList<Scope *> _visibleScopes;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
