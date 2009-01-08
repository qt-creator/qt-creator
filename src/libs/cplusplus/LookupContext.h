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

#ifndef CPLUSPLUS_LOOKUPCONTEXT_H
#define CPLUSPLUS_LOOKUPCONTEXT_H

#include <cplusplus/CppDocument.h>
#include <QPair>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext(Control *control = 0);

    LookupContext(Symbol *symbol,
                  Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &documents);

    LookupContext(Symbol *symbol,
                  const LookupContext &context);

    LookupContext(Symbol *symbol,
                  Document::Ptr thisDocument,
                  const LookupContext &context);

    bool isValid() const;
    operator bool() const;

    Control *control() const;
    Symbol *symbol() const;
    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;

    QList<Symbol *> resolve(Name *name) const
    { return resolve(name, visibleScopes()); }

    QList<Symbol *> resolveNamespace(Name *name) const
    { return resolveNamespace(name, visibleScopes()); }

    QList<Symbol *> resolveClass(Name *name) const
    { return resolveClass(name, visibleScopes()); }

    QList<Symbol *> resolveClassOrNamespace(Name *name) const
    { return resolveClassOrNamespace(name, visibleScopes()); }

    Snapshot snapshot() const
    { return _documents; }

    enum ResolveMode {
        ResolveSymbol           = 0x01,
        ResolveClass            = 0x02,
        ResolveNamespace        = 0x04,
        ResolveClassOrNamespace = ResolveClass  | ResolveNamespace,
        ResolveAll              = ResolveSymbol | ResolveClassOrNamespace
    };

    Identifier *identifier(Name *name) const;

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

    void expandNamespace(Scope *scope,
                         const QList<Scope *> &visibleScopes,
                         QList<Scope *> *expandedScopes) const;

    void expandClass(Scope *scope,
                     const QList<Scope *> &visibleScopes,
                     QList<Scope *> *expandedScopes) const;

    void expandBlock(Scope *scope,
                     const QList<Scope *> &visibleScopes,
                     QList<Scope *> *expandedScopes) const;

    void expandFunction(Scope *scope,
                        const QList<Scope *> &visibleScopes,
                        QList<Scope *> *expandedScopes) const;

private:
    QList<Scope *> buildVisibleScopes();
    static bool isNameCompatibleWithIdentifier(Name *name, Identifier *id);

private:
    Control *_control;

    // The current symbol.
    Symbol *_symbol;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _documents;

    // Visible scopes.
    QList<Scope *> _visibleScopes;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
