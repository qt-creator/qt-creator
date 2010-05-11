/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "LookupItem.h"
#include <FullySpecifiedType.h>
#include <Type.h>
#include <SymbolVisitor.h>
#include <QtCore/QSet>
#include <map>
#include <functional>

namespace CPlusPlus {

class CreateBindings;

class CPLUSPLUS_EXPORT ClassOrNamespace
{
public:
    ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent);

    ClassOrNamespace *parent() const;
    QList<ClassOrNamespace *> usings() const;
    QList<Enum *> enums() const;
    QList<Symbol *> symbols() const;

    ClassOrNamespace *globalNamespace() const;

    QList<Symbol *> lookup(const Name *name);

    ClassOrNamespace *lookupClassOrNamespace(const Name *name);
    ClassOrNamespace *findClassOrNamespace(const Name *name);
    ClassOrNamespace *findClassOrNamespace(const QList<const Name *> &path);

private:
    /// \internal
    void flush();

    /// \internal
    ClassOrNamespace *findOrCreate(const Name *name);

    void addTodo(Symbol *symbol);
    void addSymbol(Symbol *symbol);
    void addEnum(Enum *e);
    void addUsing(ClassOrNamespace *u);
    void addNestedClassOrNamespace(const Name *alias, ClassOrNamespace *e);

    void lookup_helper(const Name *name, ClassOrNamespace *binding,
                       QList<Symbol *> *result,
                       QSet<ClassOrNamespace *> *processed,
                       const TemplateNameId *templateId);

    ClassOrNamespace *lookupClassOrNamespace_helper(const Name *name, QSet<ClassOrNamespace *> *processed);
    ClassOrNamespace *findClassOrNamespace_helper(const Name *name, QSet<ClassOrNamespace *> *processed);
    ClassOrNamespace *nestedClassOrNamespace(const Name *name) const;

private:
    struct CompareName: std::binary_function<const Name *, const Name *, bool> {
        bool operator()(const Name *name, const Name *other) const;
    };

private:
    typedef std::map<const Name *, ClassOrNamespace *, CompareName> Table;
    CreateBindings *_factory;
    ClassOrNamespace *_parent;
    QList<Symbol *> _symbols;
    QList<ClassOrNamespace *> _usings;
    Table _classOrNamespaces;
    QList<Enum *> _enums;
    QList<Symbol *> _todo;

    // it's an instantiation.
    const TemplateNameId *_templateId;

    // templates
    QList<ClassOrNamespace *> _instantiations;

    friend class CreateBindings;
};

class CPLUSPLUS_EXPORT CreateBindings: protected SymbolVisitor
{
    Q_DISABLE_COPY(CreateBindings)

public:
    CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot);
    virtual ~CreateBindings();

    /// Returns the binding for the global namespace.
    ClassOrNamespace *globalNamespace() const;

    /// Finds the binding associated to the given symbol.
    ClassOrNamespace *findClassOrNamespace(Symbol *symbol);

    /// Find the binding with the given path.
    /// \internal
    ClassOrNamespace *findClassOrNamespace(const QList<const Name *> &path);

    /// Returns the Control that must be used to create temporary symbols.
    /// \internal
    Control *control() const;

    /// Searches in \a scope for symbols with the given \a name.
    /// Store the result in \a results.
    /// \internal
    void lookupInScope(const Name *name, Scope *scope, QList<Symbol *> *result,
                       const TemplateNameId *templateId);

    /// Create bindings for the symbols reachable from \a rootSymbol.
    /// \internal
    void process(Symbol *rootSymbol, ClassOrNamespace *classOrNamespace);

    /// Create an empty ClassOrNamespace binding with the given \a parent.
    /// \internal
    ClassOrNamespace *allocClassOrNamespace(ClassOrNamespace *parent);

protected:
    using SymbolVisitor::visit;

    /// Change the current ClassOrNamespace binding.
    ClassOrNamespace *switchCurrentClassOrNamespace(ClassOrNamespace *classOrNamespace);

    /// Enters the ClassOrNamespace binding associated with the given \a symbol.
    ClassOrNamespace *enterClassOrNamespaceBinding(Symbol *symbol);

    /// Enters a ClassOrNamespace binding for the given \a symbol in the global
    /// namespace binding.
    ClassOrNamespace *enterGlobalClassOrNamespace(Symbol *symbol);

    /// Creates bindings for the given \a document.
    void process(Document::Ptr document);

    /// Creates bindings for the symbols reachable from the \a root symbol.
    void process(Symbol *root);

    virtual bool visit(Namespace *ns);
    virtual bool visit(Class *klass);
    virtual bool visit(ForwardClassDeclaration *klass);
    virtual bool visit(Enum *e);
    virtual bool visit(Declaration *decl);
    virtual bool visit(Function *);
    virtual bool visit(BaseClass *b);
    virtual bool visit(UsingNamespaceDirective *u);
    virtual bool visit(NamespaceAlias *a);

    virtual bool visit(ObjCClass *klass);
    virtual bool visit(ObjCBaseClass *b);
    virtual bool visit(ObjCForwardClassDeclaration *klass);
    virtual bool visit(ObjCProtocol *proto);
    virtual bool visit(ObjCBaseProtocol *b);
    virtual bool visit(ObjCForwardProtocolDeclaration *proto);
    virtual bool visit(ObjCMethod *);

private:
    Control *_control;
    Snapshot _snapshot;
    QSet<Namespace *> _processed;
    QList<ClassOrNamespace *> _entities;
    ClassOrNamespace *_globalNamespace;
    ClassOrNamespace *_currentClassOrNamespace;
};

class CPLUSPLUS_EXPORT LookupContext
{
public:
    LookupContext();

    LookupContext(Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(Document::Ptr expressionDocument,
                  Document::Ptr thisDocument,
                  const Snapshot &snapshot);

    LookupContext(const LookupContext &other);
    LookupContext &operator = (const LookupContext &other);

    bool isValid() const;

    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    QList<Symbol *> lookup(const Name *name, Symbol *lastVisibleSymbol) const;
    QList<Symbol *> lookup(const Name *name, Scope *scope) const;

    ClassOrNamespace *globalNamespace() const;

    ClassOrNamespace *classOrNamespace(const Name *name, Symbol *lastVisibleSymbol) const;
    ClassOrNamespace *classOrNamespace(const Name *name, Scope *scope) const;
    ClassOrNamespace *classOrNamespace(Symbol *symbol) const;

    /// \internal
    QSharedPointer<CreateBindings> bindings() const;

    /// \internal
    void setBindings(QSharedPointer<CreateBindings> bindings);

    Control *control() const; // ### deprecate

private:
    Control *_control;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Bindings
    mutable QSharedPointer<CreateBindings> _bindings;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
