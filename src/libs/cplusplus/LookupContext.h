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
} // namespace Internal;

class CreateBindings;

class CPLUSPLUS_EXPORT ClassOrNamespace
{
    Q_DISABLE_COPY(ClassOrNamespace)

    ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent);

public:
    ~ClassOrNamespace();

    const TemplateNameId *templateId() const;
    ClassOrNamespace *instantiationOrigin() const;

    ClassOrNamespace *parent() const;
    QList<ClassOrNamespace *> usings() const;
    QList<Enum *> unscopedEnums() const;
    QList<Symbol *> symbols() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name);
    QList<LookupItem> find(const Name *name);

    ClassOrNamespace *lookupType(const Name *name);
    ClassOrNamespace *lookupType(const Name *name, Block *block);
    ClassOrNamespace *findType(const Name *name);
    ClassOrNamespace *findBlock(Block *block);

    Symbol *lookupInScope(const QList<const Name *> &fullName);

    /// The class this ClassOrNamespace is based on.
    Class *rootClass() const { return _rootClass; }

private:
    typedef std::map<const Name *, ClassOrNamespace *, Name::Compare> Table;
    typedef std::map<const TemplateNameId *, ClassOrNamespace *, TemplateNameId::Compare> TemplateNameIdTable;
    typedef QHash<const AnonymousNameId *, ClassOrNamespace *> Anonymouses;

    /// \internal
    void flush();

    /// \internal
    ClassOrNamespace *findOrCreateType(const Name *name, ClassOrNamespace *origin = 0,
                                       Class *clazz = 0);

    ClassOrNamespace *findOrCreateNestedAnonymousType(const AnonymousNameId *anonymousNameId);

    void addTodo(Symbol *symbol);
    void addSymbol(Symbol *symbol);
    void addUnscopedEnum(Enum *e);
    void addUsing(ClassOrNamespace *u);
    void addNestedType(const Name *alias, ClassOrNamespace *e);

    QList<LookupItem> lookup_helper(const Name *name, bool searchInEnclosingScope);

    void lookup_helper(const Name *name, ClassOrNamespace *binding,
                       QList<LookupItem> *result,
                       QSet<ClassOrNamespace *> *processed,
                       const TemplateNameId *templateId);

    ClassOrNamespace *lookupType_helper(const Name *name, QSet<ClassOrNamespace *> *processed,
                                        bool searchInEnclosingScope, ClassOrNamespace *origin);

    ClassOrNamespace *findBlock_helper(Block *block, QSet<ClassOrNamespace *> *processed,
                                       bool searchInEnclosingScope);

    ClassOrNamespace *nestedType(const Name *name, ClassOrNamespace *origin);

    void instantiateNestedClasses(ClassOrNamespace *enclosingTemplateClass,
                                  Clone &cloner,
                                  Subst &subst,
                                  ClassOrNamespace *enclosingTemplateClassInstantiation);
    ClassOrNamespace *findSpecialization(const TemplateNameId *templId,
                                         const TemplateNameIdTable &specializations);

    CreateBindings *_factory;
    ClassOrNamespace *_parent;
    QList<Symbol *> _symbols;
    QList<ClassOrNamespace *> _usings;
    Table _classOrNamespaces;
    QHash<Block *, ClassOrNamespace *> _blocks;
    QList<Enum *> _enums;
    QList<Symbol *> _todo;
    QSharedPointer<Control> _control;
    TemplateNameIdTable _specializations;
    QMap<const TemplateNameId *, ClassOrNamespace *> _instantiations;
    Anonymouses _anonymouses;
    QSet<const AnonymousNameId *> _declaredOrTypedefedAnonymouses;

    QHash<Internal::FullyQualifiedName, Symbol *> *_scopeLookupCache;

    // it's an instantiation.
    const TemplateNameId *_templateId;
    ClassOrNamespace *_instantiationOrigin;

    AlreadyConsideredClassContainer<Class> _alreadyConsideredClasses;
    AlreadyConsideredClassContainer<TemplateNameId> _alreadyConsideredTemplates;

    Class *_rootClass;

    class NestedClassInstantiator
    {
    public:
        NestedClassInstantiator(CreateBindings *factory, Clone &cloner, Subst &subst)
            : _factory(factory)
            , _cloner(cloner)
            , _subst(subst)
        {}
        void instantiate(ClassOrNamespace *enclosingTemplateClass,
                         ClassOrNamespace *enclosingTemplateClassInstantiation);
    private:
        bool isInstantiateNestedClassNeeded(const QList<Symbol *> &symbols) const;
        bool containsTemplateType(Declaration *declaration) const;
        bool containsTemplateType(Function *function) const;
        NamedType *findNamedType(Type *memberType) const;

        QSet<ClassOrNamespace *> _alreadyConsideredNestedClassInstantiations;
        CreateBindings *_factory;
        Clone &_cloner;
        Subst &_subst;
    };

public:
    const Name *_name; // For debug

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
    ClassOrNamespace *lookupType(Symbol *symbol, ClassOrNamespace *enclosingBinding = 0);
    ClassOrNamespace *lookupType(const QList<const Name *> &path,
                                 ClassOrNamespace *enclosingBinding = 0);

    /// Returns the Control that must be used to create temporary symbols.
    /// \internal
    QSharedPointer<Control> control() const
    { return _control; }

    bool expandTemplates() const
    { return _expandTemplates; }
    void setExpandTemplates(bool expandTemplates)
    { _expandTemplates = expandTemplates; }

    /// Searches in \a scope for symbols with the given \a name.
    /// Store the result in \a results.
    /// \internal
    void lookupInScope(const Name *name, Scope *scope, QList<LookupItem> *result,
                            const TemplateNameId *templateId, ClassOrNamespace *binding);

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

    virtual bool visit(Template *templ);
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
    Symbol *instantiateTemplateFunction(const TemplateNameId *instantiation,
                                        Template *specialization) const;

    Snapshot _snapshot;
    QSharedPointer<Control> _control;
    QSet<Namespace *> _processed;
    QList<ClassOrNamespace *> _entities;
    ClassOrNamespace *_globalNamespace;
    ClassOrNamespace *_currentClassOrNamespace;
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
                  QSharedPointer<CreateBindings> bindings = QSharedPointer<CreateBindings>());

    LookupContext(const LookupContext &other);
    LookupContext &operator = (const LookupContext &other);

    Document::Ptr expressionDocument() const;
    Document::Ptr thisDocument() const;
    Document::Ptr document(const QString &fileName) const;
    Snapshot snapshot() const;

    ClassOrNamespace *globalNamespace() const;

    QList<LookupItem> lookup(const Name *name, Scope *scope) const;
    ClassOrNamespace *lookupType(const Name *name, Scope *scope,
                                 ClassOrNamespace *enclosingBinding = 0,
                                 QSet<const Declaration *> typedefsBeingResolved
                                    = QSet<const Declaration *>()) const;
    ClassOrNamespace *lookupType(Symbol *symbol,
                                 ClassOrNamespace *enclosingBinding = 0) const;
    ClassOrNamespace *lookupParent(Symbol *symbol) const;

    /// \internal
    QSharedPointer<CreateBindings> bindings() const
    { return _bindings; }

    static QList<const Name *> fullyQualifiedName(Symbol *symbol);
    static QList<const Name *> path(Symbol *symbol);

    static const Name *minimalName(Symbol *symbol, ClassOrNamespace *target, Control *control);

    void setExpandTemplates(bool expandTemplates)
    {
        if (_bindings)
            _bindings->setExpandTemplates(expandTemplates);
        m_expandTemplates = expandTemplates;
    }

private:
    QList<LookupItem> lookupByUsing(const Name *name, ClassOrNamespace *bindingScope) const;

    // The current expression.
    Document::Ptr _expressionDocument;

    // The current document.
    Document::Ptr _thisDocument;

    // All documents.
    Snapshot _snapshot;

    // Bindings
    QSharedPointer<CreateBindings> _bindings;

    bool m_expandTemplates;
};

bool CPLUSPLUS_EXPORT compareFullyQualifiedName(const QList<const Name *> &path,
                                                const QList<const Name *> &other);


} // namespace CPlusPlus

#endif // CPLUSPLUS_LOOKUPCONTEXT_H
