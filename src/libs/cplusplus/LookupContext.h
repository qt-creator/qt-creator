/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPLUSPLUS_LOOKUPCONTEXT_H
#define CPLUSPLUS_LOOKUPCONTEXT_H

#include "CppDocument.h"
#include "LookupItem.h"
#include "AlreadyConsideredClassContainer.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/Type.h>
#include <cplusplus/SymbolVisitor.h>
#include <cplusplus/Control.h>
#include <cplusplus/Name.h>

#include <QEnableSharedFromThis>
#include <QSet>
#include <QMap>

#include <map>
#include <functional>

namespace CPlusPlus {

namespace Internal {
struct FullyQualifiedName
{
    QList<const Name *> fqn;

    FullyQualifiedName(const QList<const Name *> &fqn)
        : fqn(fqn)
    {}
};
class LookupScopePrivate;
class Instantiator;
} // namespace Internal;

class CreateBindings;

class CPLUSPLUS_EXPORT LookupScope
{
    Q_DISABLE_COPY(LookupScope)

    LookupScope(CreateBindings *factory, LookupScope *parent);

public:
    ~LookupScope();

    LookupScope *instantiationOrigin() const;

    LookupScope *parent() const;
    QList<LookupScope *> usings() const;
    QList<Enum *> unscopedEnums() const;
    QList<Symbol *> symbols() const;

    QList<LookupItem> lookup(const Name *name);
    QList<LookupItem> find(const Name *name);

    LookupScope *lookupType(const Name *name);
    LookupScope *lookupType(const Name *name, Block *block);
    LookupScope *findType(const Name *name);
    LookupScope *findBlock(Block *block);

    /// The class this LookupScope is based on.
    Class *rootClass() const;

private:
    Internal::LookupScopePrivate *d;

    friend class Internal::LookupScopePrivate;
    friend class Internal::Instantiator;
    friend class CreateBindings;
};

class CPLUSPLUS_EXPORT CreateBindings
        : protected SymbolVisitor
        , public QEnableSharedFromThis<CreateBindings>
{
    Q_DISABLE_COPY(CreateBindings)

public:
    typedef QSharedPointer<CreateBindings> Ptr;

    CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot);
    virtual ~CreateBindings();

    /// Returns the binding for the global namespace.
    LookupScope *globalNamespace() const;

    /// Finds the binding associated to the given symbol.
    LookupScope *lookupType(Symbol *symbol, LookupScope *enclosingBinding = 0);
    LookupScope *lookupType(const QList<const Name *> &path, LookupScope *enclosingBinding = 0);

    /// Returns the Control that must be used to create temporary symbols.
    /// \internal
    QSharedPointer<Control> control() const
    { return _control; }

    Snapshot &snapshot()
    { return _snapshot; }

    /// Adds an expression document in order to keep their symbols and names alive
    void addExpressionDocument(Document::Ptr document)
    { _expressionDocuments.append(document); }

    bool expandTemplates() const
    { return _expandTemplates; }
    void setExpandTemplates(bool expandTemplates)
    { _expandTemplates = expandTemplates; }

    /// Searches in \a scope for symbols with the given \a name.
    /// Store the result in \a results.
    /// \internal
    void lookupInScope(const Name *name, Scope *scope, QList<LookupItem> *result,
                       LookupScope *binding = 0);

    /// Create bindings for the symbols reachable from \a rootSymbol.
    /// \internal
    void process(Symbol *rootSymbol, LookupScope *lookupScope);

    /// Create an empty LookupScope binding with the given \a parent.
    /// \internal
    LookupScope *allocLookupScope(LookupScope *parent, const Name *name);

    FullySpecifiedType resolveTemplateArgument(Clone &cloner, Subst &subst,
                                               LookupScope *origin,
                                               const Template *specialization,
                                               const TemplateNameId *instantiation,
                                               unsigned index);
    void initializeSubst(Clone &cloner, Subst &subst, LookupScope *origin,
                         const Template *specialization, const TemplateNameId *instantiation);

protected:
    using SymbolVisitor::visit;

    /// Change the current LookupScope binding.
    LookupScope *switchCurrentLookupScope(LookupScope *lookupScope);

    /// Enters the LookupScope binding associated with the given \a symbol.
    LookupScope *enterLookupScopeBinding(Symbol *symbol);

    /// Enters a LookupScope binding for the given \a symbol in the global
    /// namespace binding.
    LookupScope *enterGlobalLookupScope(Symbol *symbol);

    /// Creates bindings for the given \a document.
    void process(Document::Ptr document);

    /// Creates bindings for the symbols reachable from the \a root symbol.
    void process(Symbol *root);

    virtual bool visit(Template *templ);
    virtual bool visit(ExplicitInstantiation *inst);
    virtual bool visit(Namespace *ns);
    virtual bool visit(Class *klass);
    virtual bool visit(ForwardClassDeclaration *klass);
    virtual bool visit(Enum *e);
    virtual bool visit(Declaration *decl);
    virtual bool visit(Function *function);
    virtual bool visit(Block *block);

    virtual bool visit(BaseClass *b);
    virtual bool visit(UsingNamespaceDirective *u);
    virtual bool visit(UsingDeclaration *u);
    virtual bool visit(NamespaceAlias *a);

    virtual bool visit(ObjCClass *klass);
    virtual bool visit(ObjCBaseClass *b);
    virtual bool visit(ObjCForwardClassDeclaration *klass);
    virtual bool visit(ObjCProtocol *proto);
    virtual bool visit(ObjCBaseProtocol *b);
    virtual bool visit(ObjCForwardProtocolDeclaration *proto);
    virtual bool visit(ObjCMethod *);

private:
    Snapshot _snapshot;
    QSharedPointer<Control> _control;
    QList<Document::Ptr> _expressionDocuments;
    QSet<Namespace *> _processed;
    QList<LookupScope *> _entities;
    LookupScope *_globalNamespace;
    LookupScope *_currentLookupScope;
    bool _expandTemplates;
};

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext();

    LookupContext(Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &snapshot,
                  CreateBindings::Ptr bindings = CreateBindings::Ptr());

    LookupContext(const LookupContext &other);
    LookupContext &operator = (const LookupContext &other);

    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    LookupScope *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name, Scope *scope) const;
    LookupScope *lookupType(const Name *name, Scope *scope,
                                 LookupScope *enclosingBinding = 0,
                                 QSet<const Declaration *> typedefsBeingResolved
                                    = QSet<const Declaration *>()) const;
    LookupScope *lookupType(Symbol *symbol,
                                 LookupScope *enclosingBinding = 0) const;
    LookupScope *lookupParent(Symbol *symbol) const;

    /// \internal
    CreateBindings::Ptr bindings() const
    { return _bindings; }

    static QList<const Name *> fullyQualifiedName(Symbol *symbol);
    static QList<const Name *> path(Symbol *symbol);

    static const Name *minimalName(Symbol *symbol, LookupScope *target, Control *control);

    void setExpandTemplates(bool expandTemplates)
    {
        if (_bindings)
            _bindings->setExpandTemplates(expandTemplates);
        m_expandTemplates = expandTemplates;
    }

private:
    QList<LookupItem> lookupByUsing(const Name *name, LookupScope *bindingScope) const;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Bindings
    CreateBindings::Ptr _bindings;

    bool m_expandTemplates;
};

bool CPLUSPLUS_EXPORT compareFullyQualifiedName(const QList<const Name *> &path,
                                                const QList<const Name *> &other);


} // namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
