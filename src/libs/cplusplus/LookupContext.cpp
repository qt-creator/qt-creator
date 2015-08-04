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

#include "LookupContext.h"

#include "ResolveExpression.h"
#include "Overview.h"
#include "CppRewriter.h"
#include "TypeResolver.h"

#include <cplusplus/CoreTypes.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Control.h>
#include <cplusplus/cppassert.h>

#include <QStack>
#include <QHash>
#include <QVarLengthArray>
#include <QDebug>

static const bool debug = ! qgetenv("QTC_LOOKUPCONTEXT_DEBUG").isEmpty();

namespace CPlusPlus {

typedef QSet<Internal::LookupScopePrivate *> ProcessedSet;

static void addNames(const Name *name, QList<const Name *> *names, bool addAllNames = false)
{
    if (! name)
        return;
    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        addNames(q->base(), names);
        addNames(q->name(), names, addAllNames);
    } else if (addAllNames || name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId()) {
        names->append(name);
    }
}

static void path_helper(Symbol *symbol, QList<const Name *> *names)
{
    if (! symbol)
        return;

    path_helper(symbol->enclosingScope(), names);

    if (symbol->name()) {
        if (symbol->isClass() || symbol->isNamespace()) {
            addNames(symbol->name(), names);

        } else if (symbol->isObjCClass() || symbol->isObjCBaseClass() || symbol->isObjCProtocol()
                || symbol->isObjCForwardClassDeclaration() || symbol->isObjCForwardProtocolDeclaration()
                || symbol->isForwardClassDeclaration()) {
            addNames(symbol->name(), names);

        } else if (symbol->isFunction()) {
            if (const QualifiedNameId *q = symbol->name()->asQualifiedNameId())
                addNames(q->base(), names);
        } else if (Enum *e = symbol->asEnum()) {
            if (e->isScoped())
                addNames(symbol->name(), names);
        }
    }
}

static inline bool compareName(const Name *name, const Name *other)
{
    if (name == other)
        return true;

    if (name && other) {
        const Identifier *id = name->identifier();
        const Identifier *otherId = other->identifier();

        if (id == otherId || (id && id->match(otherId)))
            return true;
    }

    return false;
}

bool compareFullyQualifiedName(const QList<const Name *> &path, const QList<const Name *> &other)
{
    if (path.length() != other.length())
        return false;

    for (int i = 0; i < path.length(); ++i) {
        if (! compareName(path.at(i), other.at(i)))
            return false;
    }

    return true;
}

namespace Internal {

bool operator==(const FullyQualifiedName &left, const FullyQualifiedName &right)
{
    return compareFullyQualifiedName(left.fqn, right.fqn);
}

uint qHash(const FullyQualifiedName &fullyQualifiedName)
{
    uint h = 0;
    for (int i = 0; i < fullyQualifiedName.fqn.size(); ++i) {
        if (const Name *n = fullyQualifiedName.fqn.at(i)) {
            if (const Identifier *id = n->identifier()) {
                h <<= 1;
                h += id->hashCode();
            }
        }
    }
    return h;
}
}

/////////////////////////////////////////////////////////////////////
// LookupContext
/////////////////////////////////////////////////////////////////////
LookupContext::LookupContext()
    : m_expandTemplates(false)
{ }

LookupContext::LookupContext(Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _expressionDocument(Document::create(QLatin1String("<LookupContext>")))
    , _thisDocument(thisDocument)
    , _snapshot(snapshot)
    , _bindings(new CreateBindings(thisDocument, snapshot))
    , m_expandTemplates(false)
{
}

LookupContext::LookupContext(Document::Ptr expressionDocument,
                             Document::Ptr thisDocument,
                             const Snapshot &snapshot,
                             CreateBindings::Ptr bindings)
    : _expressionDocument(expressionDocument)
    , _thisDocument(thisDocument)
    , _snapshot(snapshot)
    , _bindings(bindings)
    , m_expandTemplates(false)
{
}

LookupContext::LookupContext(const LookupContext &other)
    : _expressionDocument(other._expressionDocument)
    , _thisDocument(other._thisDocument)
    , _snapshot(other._snapshot)
    , _bindings(other._bindings)
    , m_expandTemplates(other.m_expandTemplates)
{ }

LookupContext &LookupContext::operator=(const LookupContext &other)
{
    _expressionDocument = other._expressionDocument;
    _thisDocument = other._thisDocument;
    _snapshot = other._snapshot;
    _bindings = other._bindings;
    m_expandTemplates = other.m_expandTemplates;
    return *this;
}

QList<const Name *> LookupContext::fullyQualifiedName(Symbol *symbol)
{
    QList<const Name *> qualifiedName = path(symbol->enclosingScope());
    addNames(symbol->name(), &qualifiedName, /*add all names*/ true);
    return qualifiedName;
}

QList<const Name *> LookupContext::path(Symbol *symbol)
{
    QList<const Name *> names;
    path_helper(symbol, &names);
    return names;
}

static bool symbolIdentical(Symbol *s1, Symbol *s2)
{
    if (!s1 || !s2)
        return false;
    if (s1->line() != s2->line())
        return false;
    if (s1->column() != s2->column())
        return false;

    return QByteArray(s1->fileName()) == QByteArray(s2->fileName());
}

const Name *LookupContext::minimalName(Symbol *symbol, LookupScope *target, Control *control)
{
    const Name *n = 0;
    QList<const Name *> names = LookupContext::fullyQualifiedName(symbol);

    for (int i = names.size() - 1; i >= 0; --i) {
        if (! n)
            n = names.at(i);
        else
            n = control->qualifiedNameId(names.at(i), n);

        // once we're qualified enough to get the same symbol, break
        if (target) {
            const QList<LookupItem> tresults = target->lookup(n);
            foreach (const LookupItem &tr, tresults) {
                if (symbolIdentical(tr.declaration(), symbol))
                    return n;
            }
        }
    }

    return n;
}

QList<LookupItem> LookupContext::lookupByUsing(const Name *name,
                                               LookupScope *bindingScope) const
{
    QList<LookupItem> candidates;
    // if it is a nameId there can be a using declaration for it
    if (name->isNameId() || name->isTemplateNameId()) {
        foreach (Symbol *s, bindingScope->symbols()) {
            if (Scope *scope = s->asScope()) {
                for (unsigned i = 0, count = scope->memberCount(); i < count; ++i) {
                    if (UsingDeclaration *u = scope->memberAt(i)->asUsingDeclaration()) {
                        if (const Name *usingDeclarationName = u->name()) {
                            if (const QualifiedNameId *q
                                    = usingDeclarationName->asQualifiedNameId()) {
                                if (q->name() && q->identifier() && name->identifier()
                                        && q->name()->identifier()->match(name->identifier())) {
                                    candidates = bindings()->globalNamespace()->find(q);

                                    // if it is not a global scope(scope of scope is not equal 0)
                                    // then add current using declaration as a candidate
                                    if (scope->enclosingScope()) {
                                        LookupItem item;
                                        item.setDeclaration(u);
                                        item.setScope(scope);
                                        candidates.append(item);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        foreach (Symbol *s, bindingScope->symbols()) {
            if (Scope *scope = s->asScope()) {
                LookupScope *base = lookupType(q->base(), scope);
                if (base)
                    candidates = lookupByUsing(q->name(), base);
                if (!candidates.isEmpty())
                    return candidates;
            }
        }
    }
    return candidates;
}


Document::Ptr LookupContext::expressionDocument() const
{ return _expressionDocument; }

Document::Ptr LookupContext::thisDocument() const
{ return _thisDocument; }

Document::Ptr LookupContext::document(const QString &fileName) const
{ return _snapshot.document(fileName); }

Snapshot LookupContext::snapshot() const
{ return _snapshot; }

LookupScope *LookupContext::globalNamespace() const
{
    return bindings()->globalNamespace();
}

LookupScope *LookupContext::lookupType(const Name *name, Scope *scope,
                                       LookupScope *enclosingBinding,
                                       QSet<const Declaration *> typedefsBeingResolved) const
{
    if (! scope || ! name) {
        return 0;
    } else if (Block *block = scope->asBlock()) {
        for (unsigned i = 0; i < block->memberCount(); ++i) {
            Symbol *m = block->memberAt(i);
            if (UsingNamespaceDirective *u = m->asUsingNamespaceDirective()) {
                if (LookupScope *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                    if (LookupScope *r = uu->lookupType(name))
                        return r;
                }
            } else if (Declaration *d = m->asDeclaration()) {
                if (d->name() && d->name()->match(name->asNameId())) {
                    if (d->isTypedef() && d->type()) {
                        if (Q_UNLIKELY(debug)) {
                            Overview oo;
                            qDebug() << "Looks like" << oo(name) << "is a typedef for" << oo(d->type());
                        }
                        if (const NamedType *namedTy = d->type()->asNamedType()) {
                            // Stop on recursive typedef declarations
                            if (typedefsBeingResolved.contains(d))
                                return 0;
                            return lookupType(namedTy->name(), scope, 0,
                                              QSet<const Declaration *>(typedefsBeingResolved)
                                                << d);
                        }
                    }
                }
            } else if (UsingDeclaration *ud = m->asUsingDeclaration()) {
                if (name->isNameId()) {
                    if (const Name *usingDeclarationName = ud->name()) {
                        if (const QualifiedNameId *q = usingDeclarationName->asQualifiedNameId()) {
                            if (q->name() && q->name()->match(name))
                                return bindings()->globalNamespace()->lookupType(q);
                        }
                    }
                }
            }
        }
        // try to find it in block (rare case but has priority before enclosing scope)
        // e.g.: void foo() { struct S {};  S s; }
        if (LookupScope *b = bindings()->lookupType(scope, enclosingBinding)) {
            if (LookupScope *lookupScopeNestedInNestedBlock = b->lookupType(name, block))
                return lookupScopeNestedInNestedBlock;
        }

        // try to find type in enclosing scope(typical case)
        if (LookupScope *found = lookupType(name, scope->enclosingScope()))
            return found;

    } else if (LookupScope *b = bindings()->lookupType(scope, enclosingBinding)) {
        return b->lookupType(name);
    } else if (Class *scopeAsClass = scope->asClass()) {
        if (scopeAsClass->enclosingScope()->isBlock()) {
            if (LookupScope *b = lookupType(scopeAsClass->name(),
                                                 scopeAsClass->enclosingScope(),
                                                 enclosingBinding,
                                                 typedefsBeingResolved)) {
                return b->lookupType(name);
            }
        }
    }

    return 0;
}

LookupScope *LookupContext::lookupType(Symbol *symbol, LookupScope *enclosingBinding) const
{
    return bindings()->lookupType(symbol, enclosingBinding);
}

QList<LookupItem> LookupContext::lookup(const Name *name, Scope *scope) const
{
    QList<LookupItem> candidates;

    if (! name)
        return candidates;

    for (; scope; scope = scope->enclosingScope()) {
        if (name->identifier() != 0 && scope->isBlock()) {
            bindings()->lookupInScope(name, scope, &candidates);

            if (! candidates.isEmpty()) {
                // it's a local.
                //for qualified it can be outside of the local scope
                if (name->isQualifiedNameId())
                    continue;
                else
                    break;
            }

            for (unsigned i = 0; i < scope->memberCount(); ++i) {
                if (UsingNamespaceDirective *u = scope->memberAt(i)->asUsingNamespaceDirective()) {
                    if (LookupScope *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                        candidates = uu->find(name);

                        if (! candidates.isEmpty())
                            return candidates;
                    }
                }
            }

            if (LookupScope *bindingScope = bindings()->lookupType(scope)) {
                if (LookupScope *bindingBlock = bindingScope->findBlock(scope->asBlock())) {
                    candidates = lookupByUsing(name, bindingBlock);
                    if (! candidates.isEmpty())
                        return candidates;

                    candidates = bindingBlock->find(name);

                    if (! candidates.isEmpty())
                        return candidates;
                }
            }

        } else if (Function *fun = scope->asFunction()) {
            bindings()->lookupInScope(name, fun, &candidates);

            if (! candidates.isEmpty()) {
                // it's an argument or a template parameter.
                //for qualified it can be outside of the local scope
                if (name->isQualifiedNameId())
                    continue;
                else
                    break;
            }

            if (fun->name() && fun->name()->isQualifiedNameId()) {
                if (LookupScope *binding = bindings()->lookupType(fun)) {
                    candidates = binding->find(name);

                    // try find this name in parent class
                    while (candidates.isEmpty() && (binding = binding->parent()))
                        candidates = binding->find(name);

                    if (! candidates.isEmpty())
                        return candidates;
                }
            }

            // continue, and look at the enclosing scope.

        } else if (ObjCMethod *method = scope->asObjCMethod()) {
            bindings()->lookupInScope(name, method, &candidates);

            if (! candidates.isEmpty())
                break; // it's a formal argument.

        } else if (Template *templ = scope->asTemplate()) {
            bindings()->lookupInScope(name, templ, &candidates);

            if (! candidates.isEmpty()) {
                // it's a template parameter.
                //for qualified it can be outside of the local scope
                if (name->isQualifiedNameId())
                    continue;
                else
                    break;
            }

        } else if (scope->asNamespace()
                   || scope->asClass()
                   || (scope->asEnum() && scope->asEnum()->isScoped())) {

            if (LookupScope *bindingScope = bindings()->lookupType(scope)) {
                candidates = bindingScope->find(name);

                if (! candidates.isEmpty())
                    return candidates;

                candidates = lookupByUsing(name, bindingScope);
                if (!candidates.isEmpty())
                    return candidates;
            }

            // the scope can be defined inside a block, try to find it
            if (Block *block = scope->enclosingBlock()) {
                if (LookupScope *b = bindings()->lookupType(block)) {
                    if (LookupScope *lookupScopeNestedInNestedBlock = b->lookupType(scope->name(), block))
                        candidates = lookupScopeNestedInNestedBlock->find(name);
                }
            }

            if (! candidates.isEmpty())
                return candidates;

        } else if (scope->isObjCClass() || scope->isObjCProtocol()) {
            if (LookupScope *binding = bindings()->lookupType(scope))
                candidates = binding->find(name);

                if (! candidates.isEmpty())
                    return candidates;
        }
    }

    return candidates;
}

LookupScope *LookupContext::lookupParent(Symbol *symbol) const
{
    QList<const Name *> fqName = path(symbol);
    LookupScope *binding = globalNamespace();
    foreach (const Name *name, fqName) {
        binding = binding->findType(name);
        if (!binding)
            return 0;
    }

    return binding;
}

namespace Internal {

class LookupScopePrivate
{
public:
    LookupScopePrivate(LookupScope *q, CreateBindings *factory, LookupScope *parent);
    ~LookupScopePrivate();

    typedef std::map<const Name *, LookupScopePrivate *, Name::Compare> Table;
    typedef std::map<const Name *, Declaration *, Name::Compare> TypedefTable;
    typedef std::map<const TemplateNameId *,
                     LookupScopePrivate *,
                     TemplateNameId::Compare> TemplateNameIdTable;
    typedef QHash<const AnonymousNameId *, LookupScopePrivate *> Anonymouses;

    LookupScopePrivate *allocateChild(const Name *name);

    void flush();

    LookupScope *globalNamespace() const;

    Symbol *lookupInScope(const QList<const Name *> &fullName);

    LookupScope *findOrCreateType(const Name *name, LookupScopePrivate *origin = 0,
                                       Class *clazz = 0);

    LookupScopePrivate *findOrCreateNestedAnonymousType(const AnonymousNameId *anonymousNameId);

    void addTodo(Symbol *symbol);
    void addSymbol(Symbol *symbol);
    void addUnscopedEnum(Enum *e);
    void addTypedef(const Name *identifier, Declaration *d);
    void addUsing(LookupScope *u);
    void addNestedType(const Name *alias, LookupScope *e);

    QList<LookupItem> lookup_helper(const Name *name, bool searchInEnclosingScope);

    void lookup_helper(const Name *name, LookupScopePrivate *binding,
                       QList<LookupItem> *result,
                       ProcessedSet *processed);

    LookupScope *lookupType_helper(const Name *name, ProcessedSet *processed,
                                   bool searchInEnclosingScope, LookupScopePrivate *origin);

    LookupScope *findBlock_helper(Block *block, ProcessedSet *processed,
                                  bool searchInEnclosingScope);

private:
    LookupScopePrivate *findNestedType(const Name *name, LookupScopePrivate *origin);

    LookupScopePrivate *nestedType(const Name *name, LookupScopePrivate *origin);

    LookupScopePrivate *findSpecialization(const Template *baseTemplate, const TemplateNameId *templId,
                                           const TemplateNameIdTable &specializations,
                                           LookupScopePrivate *origin);

public:
    LookupScope *q;

    CreateBindings *_factory;
    LookupScopePrivate *_parent;
    QList<Symbol *> _symbols;
    QList<LookupScope *> _usings;
    Table _nestedScopes;
    TypedefTable _typedefs;
    QHash<Block *, LookupScope *> _blocks;
    QList<Enum *> _enums;
    QList<Symbol *> _todo;
    QSharedPointer<Control> _control;
    TemplateNameIdTable _specializations;
    QMap<const TemplateNameId *, LookupScopePrivate *> _instantiations;
    Anonymouses _anonymouses;
    QSet<const AnonymousNameId *> _declaredOrTypedefedAnonymouses;

    QHash<Internal::FullyQualifiedName, Symbol *> *_scopeLookupCache;

    // it's an instantiation.
    LookupScopePrivate *_instantiationOrigin;

    AlreadyConsideredClassContainer<Class> _alreadyConsideredClasses;
    AlreadyConsideredClassContainer<TemplateNameId> _alreadyConsideredTemplates;
    QSet<const Declaration *> _alreadyConsideredTypedefs;

    Class *_rootClass;
    const Name *_name;
    bool _hasTypedefs;
};

class Instantiator
{
public:
    Instantiator(Clone &cloner, Subst &subst)
        : _cloner(cloner)
        , _subst(subst)
    {}
    void doInstantiate(LookupScopePrivate *lookupScope, LookupScopePrivate *instantiation);
    LookupScopePrivate *instantiate(LookupScopePrivate *lookupScope, LookupScopePrivate *origin);

private:
    ProcessedSet _alreadyConsideredInstantiations;
    Clone &_cloner;
    Subst &_subst;
};

LookupScopePrivate::LookupScopePrivate(LookupScope *q, CreateBindings *factory, LookupScope *parent)
    : q(q)
    , _factory(factory)
    , _parent(parent ? parent->d : 0)
    , _scopeLookupCache(0)
    , _instantiationOrigin(0)
    , _rootClass(0)
    , _name(0)
    , _hasTypedefs(false)
{
    Q_ASSERT(factory);
}

LookupScopePrivate::~LookupScopePrivate()
{
    delete _scopeLookupCache;
}

LookupScopePrivate *LookupScopePrivate::allocateChild(const Name *name)
{
    LookupScope *e = _factory->allocLookupScope(q, name);
    return e->d;
}

} // namespace Internal

LookupScope::LookupScope(CreateBindings *factory, LookupScope *parent)
    : d(new Internal::LookupScopePrivate(this, factory, parent))
{
}

LookupScope::~LookupScope()
{
    delete d;
}

LookupScope *LookupScope::instantiationOrigin() const
{
    if (Internal::LookupScopePrivate *i = d->_instantiationOrigin)
        return i->q;
    return 0;
}

LookupScope *LookupScope::parent() const
{
    if (Internal::LookupScopePrivate *p = d->_parent)
        return p->q;
    return 0;
}

QList<LookupScope *> LookupScope::usings() const
{
    const_cast<LookupScope *>(this)->d->flush();
    return d->_usings;
}

QList<Enum *> LookupScope::unscopedEnums() const
{
    const_cast<LookupScope *>(this)->d->flush();
    return d->_enums;
}

QList<Symbol *> LookupScope::symbols() const
{
    const_cast<LookupScope *>(this)->d->flush();
    return d->_symbols;
}

QList<LookupItem> LookupScope::find(const Name *name)
{
    return d->lookup_helper(name, false);
}

QList<LookupItem> LookupScope::lookup(const Name *name)
{
    return d->lookup_helper(name, true);
}

namespace Internal {

LookupScope *LookupScopePrivate::globalNamespace() const
{
    const LookupScopePrivate *e = this;

    do {
        if (! e->_parent)
            break;

        e = e->_parent;
    } while (e);

    return e ? e->q : 0;
}

QList<LookupItem> LookupScopePrivate::lookup_helper(const Name *name, bool searchInEnclosingScope)
{
    QList<LookupItem> result;

    if (name) {

        if (const QualifiedNameId *qName = name->asQualifiedNameId()) {
            if (! qName->base()) { // e.g. ::std::string
                result = globalNamespace()->find(qName->name());
            } else if (LookupScope *binding = q->lookupType(qName->base())) {
                result = binding->find(qName->name());

                QList<const Name *> fullName;
                addNames(name, &fullName);

                // It's also possible that there are matches in the parent binding through
                // a qualified name. For instance, a nested class which is forward declared
                // in the class but defined outside it - we should capture both.
                Symbol *match = 0;
                ProcessedSet processed;
                for (LookupScopePrivate *parentBinding = binding->d->_parent;
                        parentBinding && !match;
                        parentBinding = parentBinding->_parent) {
                    if (processed.contains(parentBinding))
                        break;
                    processed.insert(parentBinding);
                    match = parentBinding->lookupInScope(fullName);
                }

                if (match) {
                    LookupItem item;
                    item.setDeclaration(match);
                    item.setBinding(binding);
                    result.append(item);
                }
            }

            return result;
        }

        ProcessedSet processed;
        ProcessedSet processedOwnParents;
        LookupScopePrivate *binding = this;
        do {
            if (processedOwnParents.contains(binding))
                break;
            processedOwnParents.insert(binding);
            lookup_helper(name, binding, &result, &processed);
            binding = binding->_parent;
        } while (searchInEnclosingScope && binding);
    }

    return result;
}

void LookupScopePrivate::lookup_helper(
        const Name *name, LookupScopePrivate *binding, QList<LookupItem> *result,
        ProcessedSet *processed)
{
    if (!binding || processed->contains(binding))
        return;
    processed->insert(binding);

    binding->flush();
    const Identifier *nameId = name->identifier();

    foreach (Symbol *s, binding->_symbols) {
        if (s->isFriend())
            continue;
        else if (s->isUsingNamespaceDirective())
            continue;


        if (Scope *scope = s->asScope()) {
            if (Class *klass = scope->asClass()) {
                if (const Identifier *id = klass->identifier()) {
                    if (nameId && nameId->match(id)) {
                        LookupItem item;
                        item.setDeclaration(klass);
                        item.setBinding(binding->q);
                        result->append(item);
                    }
                }
            }
            _factory->lookupInScope(name, scope, result, binding->q);
        }
    }

    foreach (Enum *e, binding->_enums)
        _factory->lookupInScope(name, e, result, binding->q);

    foreach (LookupScope *u, binding->_usings)
        lookup_helper(name, u->d, result, processed);

    Anonymouses::const_iterator cit = binding->_anonymouses.constBegin();
    Anonymouses::const_iterator citEnd = binding->_anonymouses.constEnd();
    for (; cit != citEnd; ++cit) {
        const AnonymousNameId *anonymousNameId = cit.key();
        LookupScopePrivate *a = cit.value();
        if (!binding->_declaredOrTypedefedAnonymouses.contains(anonymousNameId))
            lookup_helper(name, a, result, processed);
    }
}

}

void CreateBindings::lookupInScope(const Name *name, Scope *scope,
                                   QList<LookupItem> *result,
                                   LookupScope *binding)
{
    if (! name) {
        return;

    } else if (const OperatorNameId *op = name->asOperatorNameId()) {
        for (Symbol *s = scope->find(op->kind()); s; s = s->next()) {
            if (! s->name())
                continue;
            else if (s->isFriend())
                continue;
            else if (! s->name()->match(op))
                continue;

            LookupItem item;
            item.setDeclaration(s);
            item.setBinding(binding);
            result->append(item);
        }

    } else if (const Identifier *id = name->identifier()) {
        for (Symbol *s = scope->find(id); s; s = s->next()) {
            if (s->isFriend())
                continue; // skip friends
            else if (s->isUsingNamespaceDirective())
                continue; // skip using namespace directives
            else if (! id->match(s->identifier()))
                continue;
            else if (s->name() && s->name()->isQualifiedNameId())
                continue; // skip qualified ids.

            if (Q_UNLIKELY(debug)) {
                Overview oo;
                qDebug() << "Found" << id->chars() << "in"
                         << (binding ? oo(binding->d->_name) : QString::fromLatin1("<null>"));
            }

            LookupItem item;
            item.setDeclaration(s);
            item.setBinding(binding);

            if (s->asNamespaceAlias() && binding) {
                LookupScope *targetNamespaceBinding = binding->lookupType(name);
                //there can be many namespace definitions
                if (targetNamespaceBinding && targetNamespaceBinding->symbols().size() > 0) {
                    Symbol *resolvedSymbol = targetNamespaceBinding->symbols().first();
                    item.setType(resolvedSymbol->type()); // override the type
                }
            }

            // instantiate function template
            if (const TemplateNameId *instantiation = name->asTemplateNameId()) {
                if (Template *specialization = s->asTemplate()) {
                    if (Symbol *decl = specialization->declaration()) {
                        if (decl->isFunction() || decl->isDeclaration()) {
                            Clone cloner(_control.data());
                            Subst subst(_control.data());
                            initializeSubst(cloner, subst, binding, specialization, instantiation);
                            Symbol *instantiatedFunctionTemplate = cloner.symbol(decl, &subst);
                            item.setType(instantiatedFunctionTemplate->type()); // override the type
                        }
                    }
                }
            }

            result->append(item);
        }
    }
}

LookupScope *LookupScope::lookupType(const Name *name)
{
    if (! name)
        return 0;

    ProcessedSet processed;
    return d->lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ true, d);
}

LookupScope *LookupScope::lookupType(const Name *name, Block *block)
{
    d->flush();

    QHash<Block *, LookupScope *>::const_iterator citBlock = d->_blocks.constFind(block);
    if (citBlock != d->_blocks.constEnd()) {
        LookupScope *nestedBlock = citBlock.value();
        ProcessedSet processed;
        if (LookupScope *foundInNestedBlock
                = nestedBlock->d->lookupType_helper(name,
                                                    &processed,
                                                    /*searchInEnclosingScope = */ true,
                                                    nestedBlock->d)) {
            return foundInNestedBlock;
        }
    }

    for (citBlock = d->_blocks.constBegin(); citBlock != d->_blocks.constEnd(); ++citBlock) {
        if (LookupScope *foundNestedBlock = citBlock.value()->lookupType(name, block))
            return foundNestedBlock;
    }

    return 0;
}

LookupScope *LookupScope::findType(const Name *name)
{
    ProcessedSet processed;
    return d->lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ true, d);
}

LookupScope *Internal::LookupScopePrivate::findBlock_helper(
        Block *block, ProcessedSet *processed, bool searchInEnclosingScope)
{
    for (LookupScopePrivate *binding = this; binding; binding = binding->_parent) {
        if (processed->contains(binding))
            break;
        processed->insert(binding);
        binding->flush();
        auto end = binding->_blocks.end();
        auto citBlock = binding->_blocks.find(block);
        if (citBlock != end)
            return citBlock.value();

        for (citBlock = binding->_blocks.begin(); citBlock != end; ++citBlock) {
            if (LookupScope *foundNestedBlock =
                    citBlock.value()->d->findBlock_helper(block, processed, false)) {
                return foundNestedBlock;
            }
        }
        if (!searchInEnclosingScope)
            break;
    }
    return 0;
}

LookupScope *LookupScope::findBlock(Block *block)
{
    ProcessedSet processed;
    return d->findBlock_helper(block, &processed, true);
}

Symbol *Internal::LookupScopePrivate::lookupInScope(const QList<const Name *> &fullName)
{
    if (!_scopeLookupCache) {
        _scopeLookupCache = new QHash<Internal::FullyQualifiedName, Symbol *>;

        flush();
        for (int j = 0; j < _symbols.size(); ++j) {
            if (Scope *scope = _symbols.at(j)->asScope()) {
                for (unsigned i = 0; i < scope->memberCount(); ++i) {
                    Symbol *s = scope->memberAt(i);
                    _scopeLookupCache->insert(LookupContext::fullyQualifiedName(s), s);
                }
            }
        }
    }

    return _scopeLookupCache->value(fullName, 0);
}

Class *LookupScope::rootClass() const
{
    return d->_rootClass;
}

namespace Internal {

LookupScope *LookupScopePrivate::lookupType_helper(
        const Name *name, ProcessedSet *processed,
        bool searchInEnclosingScope, LookupScopePrivate *origin)
{
    if (Q_UNLIKELY(debug)) {
        Overview oo;
        qDebug() << "Looking up" << oo(name) << "in" << oo(_name);
    }

    if (const QualifiedNameId *qName = name->asQualifiedNameId()) {

        ProcessedSet innerProcessed;
        if (! qName->base())
            return globalNamespace()->d->lookupType_helper(qName->name(), &innerProcessed, true, origin);

        if (LookupScope *binding = lookupType_helper(qName->base(), processed, true, origin))
            return binding->d->lookupType_helper(qName->name(), &innerProcessed, false, origin);

        return 0;

    } else if (! processed->contains(this)) {
        processed->insert(this);

        if (name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId()) {
            flush();

            foreach (Symbol *s, _symbols) {
                if (Class *klass = s->asClass()) {
                    if (klass->name() && klass->name()->match(name))
                        return q;
                }
            }
            foreach (Enum *e, _enums) {
                if (e->identifier() && e->identifier()->match(name->identifier()))
                    return q;
            }

            if (LookupScopePrivate *e = nestedType(name, origin))
                return e->q;

            foreach (LookupScope *u, _usings) {
                if (LookupScope *r = u->d->lookupType_helper(
                            name, processed, /*searchInEnclosingScope =*/ false, origin)) {
                    return r;
                }
            }

            if (_instantiationOrigin) {
                if (LookupScope *o = _instantiationOrigin->lookupType_helper(
                            name, processed, /*searchInEnclosingScope =*/ true, origin)) {
                    return o;
                }
            }
        }

        if (_parent && searchInEnclosingScope)
            return _parent->lookupType_helper(name, processed, searchInEnclosingScope, origin);
    }

    return 0;
}

static const NamedType *dereference(const FullySpecifiedType &type)
{
    FullySpecifiedType ty = type;
    forever {
        if (PointerType *pointer = ty->asPointerType())
            ty = pointer->elementType();
        else if (ReferenceType *reference = ty->asReferenceType())
            ty = reference->elementType();
        else if (ArrayType *array = ty->asArrayType())
            ty = array->elementType();
        else if (const NamedType *namedType = ty->asNamedType())
            return namedType;
        else
            break;
    }
    return 0;
}

static bool findTemplateArgument(const NamedType *namedType, LookupScopePrivate *reference)
{
    if (!namedType)
        return false;
    const Name *argumentName = namedType->name();
    foreach (Symbol *s, reference->_symbols) {
        if (Class *clazz = s->asClass()) {
            if (Template *templateSpecialization = clazz->enclosingTemplate()) {
                const unsigned argumentCountOfSpecialization
                                    = templateSpecialization->templateParameterCount();
                for (unsigned i = 0; i < argumentCountOfSpecialization; ++i) {
                    if (TypenameArgument *tParam
                            = templateSpecialization->templateParameterAt(i)->asTypenameArgument()) {
                        if (const Name *name = tParam->name()) {
                            if (compareName(name, argumentName))
                                return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool matchTypes(const FullySpecifiedType &instantiation,
                       const FullySpecifiedType &specialization)
{
    if (specialization.match(instantiation))
        return true;
    if (const NamedType *specName = specialization->asNamedType()) {
        if (const NamedType *initName = instantiation->asNamedType()) {
            if (specName->name()->identifier()->match(initName->name()->identifier()))
                return true;
        }
    }
    return false;
}

LookupScopePrivate *LookupScopePrivate::findSpecialization(
        const Template *baseTemplate,
        const TemplateNameId *templId,
        const TemplateNameIdTable &specializations,
        LookupScopePrivate *origin)
{
    Clone cloner(_factory->control().data());
    for (TemplateNameIdTable::const_iterator cit = specializations.begin();
         cit != specializations.end(); ++cit) {
        const TemplateNameId *specializationNameId = cit->first;
        const unsigned specializationTemplateArgumentCount
                = specializationNameId->templateArgumentCount();
        Subst subst(_factory->control().data());
        bool match = true;
        for (unsigned i = 0; i < specializationTemplateArgumentCount && match; ++i) {
            const FullySpecifiedType &specializationTemplateArgument
                    = specializationNameId->templateArgumentAt(i);
            FullySpecifiedType initializationTemplateArgument =
                    _factory->resolveTemplateArgument(cloner, subst, origin ? origin->q : 0,
                                                      baseTemplate, templId, i);
            // specialization and initialization argument have to be a pointer
            // additionally type of pointer argument of specialization has to be namedType
            if (findTemplateArgument(dereference(specializationTemplateArgument), cit->second)) {
                if (specializationTemplateArgument->isPointerType())
                    match = initializationTemplateArgument->isPointerType();
                else if (specializationTemplateArgument->isReferenceType())
                    match = initializationTemplateArgument->isReferenceType();
                else if (specializationTemplateArgument->isArrayType())
                    match = initializationTemplateArgument->isArrayType();
                // Do not try exact match (typename T != class T {};)
            } else {
                // Real type specialization
                match = matchTypes(initializationTemplateArgument, specializationTemplateArgument);
            }
        }
        if (match)
            return cit->second;
    }

    return 0;
}

LookupScopePrivate *LookupScopePrivate::findOrCreateNestedAnonymousType(
        const AnonymousNameId *anonymousNameId)
{
    auto cit = _anonymouses.constFind(anonymousNameId);
    if (cit != _anonymouses.constEnd()) {
        return cit.value();
    } else {
        LookupScopePrivate *newAnonymous = allocateChild(anonymousNameId);
        _anonymouses[anonymousNameId] = newAnonymous;
        return newAnonymous;
    }
}

LookupScopePrivate *LookupScopePrivate::findNestedType(const Name *name, LookupScopePrivate *origin)
{
    TypedefTable::const_iterator typedefit = _typedefs.find(name);
    if (typedefit != _typedefs.end()) {
        Declaration *decl = typedefit->second;
        if (_alreadyConsideredTypedefs.contains(decl))
            return 0;
        LookupScopePrivate *binding = 0;
        _alreadyConsideredTypedefs.insert(decl);
        if (const NamedType *namedTy = decl->type()->asNamedType()) {
            if (LookupScope *e = q->lookupType(namedTy->name())) {
                binding = e->d;
            } else if (origin) {
                if (LookupScope *e = origin->q->lookupType(namedTy->name()))
                    binding = e->d;
            }
        }
        _alreadyConsideredTypedefs.remove(decl);
        if (binding)
            return binding;
    }

    auto it = _nestedScopes.find(name);
    if (it != _nestedScopes.end())
        return it->second;

    return 0;
}

LookupScopePrivate *LookupScopePrivate::nestedType(const Name *name, LookupScopePrivate *origin)
{
    Q_ASSERT(name != 0);
    Q_ASSERT(name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId());

    const_cast<LookupScopePrivate *>(this)->flush();

    if (const AnonymousNameId *anonymousNameId = name->asAnonymousNameId())
        return findOrCreateNestedAnonymousType(anonymousNameId);

    LookupScopePrivate *reference = findNestedType(name, origin);
    if (!reference)
        return 0;
    reference->flush();
    LookupScopePrivate *baseTemplateClassReference = reference;

    const TemplateNameId *templId = name->asTemplateNameId();
    if (templId) {
        // for "using" we should use the real one LookupScope(it should be the first
        // one item from usings list)
        // we indicate that it is a 'using' by checking number of symbols(it should be 0)
        if (reference->_symbols.count() == 0 && reference->_usings.count() != 0)
            reference = reference->_usings[0]->d;

        // if it is a TemplateNameId it could be a specialization(full or partial) or
        // instantiation of one of the specialization(reference->_specialization) or
        // base class(reference)
        if (templId->isSpecialization()) {
            // if it is a specialization we try to find or create new one and
            // add to base class(reference)
            TemplateNameIdTable::const_iterator cit
                    = reference->_specializations.find(templId);
            if (cit != reference->_specializations.end()) {
                return cit->second;
            } else {
                LookupScopePrivate *newSpecialization = reference->allocateChild(templId);
                reference->_specializations[templId] = newSpecialization;
                return newSpecialization;
            }
        } else {
            auto citInstantiation = reference->_instantiations.constFind(templId);
            if (citInstantiation != reference->_instantiations.constEnd())
                return citInstantiation.value();
            TemplateNameId *nonConstTemplId = const_cast<TemplateNameId *>(templId);
            // make this instantiation looks like specialization which help to find
            // full specialization for this instantiation
            nonConstTemplId->setIsSpecialization(true);
            const TemplateNameIdTable &specializations = reference->_specializations;
            TemplateNameIdTable::const_iterator cit = specializations.find(templId);
            if (cit != specializations.end()) {
                // we found full specialization
                reference = cit->second;
            } else {
                Template *baseTemplate = 0;
                foreach (Symbol *s, reference->_symbols) {
                    if (Class *clazz = s->asClass())
                        baseTemplate = clazz->enclosingTemplate();
                    else if (ForwardClassDeclaration *forward = s->asForwardClassDeclaration())
                        baseTemplate = forward->enclosingTemplate();
                    if (baseTemplate)
                        break;
                }
                if (baseTemplate) {
                    if (LookupScopePrivate *specialization =
                            findSpecialization(baseTemplate, templId, specializations, origin)) {
                        reference = specialization;
                        if (Q_UNLIKELY(debug)) {
                            Overview oo;
                            qDebug() << "picked specialization" << oo(specialization->_name);
                        }
                    }
                }
            }
            // let's instantiation be instantiation
            nonConstTemplId->setIsSpecialization(false);
        }
    }

    // The reference binding might still be missing some of its base classes in the case they
    // are templates. We need to collect them now. First, we track the bases which are already
    // part of the binding so we can identify the missings ones later.

    Class *referenceClass = 0;
    QList<const Name *> allBases;
    foreach (Symbol *s, reference->_symbols) {
        if (Class *clazz = s->asClass()) {
            for (unsigned i = 0; i < clazz->baseClassCount(); ++i) {
                BaseClass *baseClass = clazz->baseClassAt(i);
                if (baseClass->name())
                    allBases.append(baseClass->name());
            }
            referenceClass = clazz;
            break;
        }
    }

    if (!referenceClass)
        return reference;

    if ((! templId && _alreadyConsideredClasses.contains(referenceClass)) ||
            (templId &&
            _alreadyConsideredTemplates.contains(templId))) {
            return reference;
    }

    if (!name->isTemplateNameId())
        _alreadyConsideredClasses.insert(referenceClass);

    QSet<LookupScope *> knownUsings = reference->_usings.toSet();

    // If we are dealling with a template type, more work is required, since we need to
    // construct all instantiation data.
    if (templId) {
        if (!_factory->expandTemplates())
            return reference;
        Template *templateSpecialization = referenceClass->enclosingTemplate();
        if (!templateSpecialization)
            return reference;

        // It gets a bit complicated if the reference is actually a class template because we
        // now must worry about dependent names in base classes.
        _alreadyConsideredTemplates.insert(templId);
        const unsigned argumentCountOfInitialization = templId->templateArgumentCount();
        const unsigned argumentCountOfSpecialization
                = templateSpecialization->templateParameterCount();

        Clone cloner(_control.data());
        Subst subst(_control.data());
        _factory->initializeSubst(cloner, subst, origin ? origin->q : 0,
                                  templateSpecialization, templId);

        LookupScopePrivate *instantiation = baseTemplateClassReference->allocateChild(templId);

        instantiation->_instantiationOrigin = origin;

        instantiation->_rootClass = reference->_rootClass;
        Instantiator instantiator(cloner, subst);
        instantiator.doInstantiate(reference, instantiation);

        QHash<const Name*, unsigned> templParams;
        for (unsigned i = 0; i < argumentCountOfSpecialization; ++i)
            templParams.insert(templateSpecialization->templateParameterAt(i)->name(), i);

        foreach (const Name *baseName, allBases) {
            LookupScope *baseBinding = 0;

            if (const Identifier *nameId = baseName->asNameId()) {
                // This is the simple case in which a template parameter is itself a base.
                // Ex.: template <class T> class A : public T {};
                if (templParams.contains(nameId)) {
                    const unsigned parameterIndex = templParams.value(nameId);
                    if (parameterIndex < argumentCountOfInitialization) {
                        const FullySpecifiedType &fullType =
                                templId->templateArgumentAt(parameterIndex);
                        if (fullType.isValid()) {
                            if (NamedType *namedType = fullType.type()->asNamedType())
                                baseBinding = q->lookupType(namedType->name());
                        }
                    }
                }
                if (!baseBinding && subst.contains(baseName)) {
                    const FullySpecifiedType &fullType = subst[baseName];
                    if (fullType.isValid()) {
                        if (NamedType *namedType = fullType.type()->asNamedType())
                            baseBinding = q->lookupType(namedType->name());
                    }
                }
            } else {
                SubstitutionMap map;
                for (unsigned i = 0; i < argumentCountOfSpecialization; ++i) {
                    const Name *name = templateSpecialization->templateParameterAt(i)->name();
                    FullySpecifiedType ty = (i < argumentCountOfInitialization) ?
                                templId->templateArgumentAt(i):
                                templateSpecialization->templateParameterAt(i)->type();

                    map.bind(name, ty);
                }
                SubstitutionEnvironment env;
                env.enter(&map);

                baseName = rewriteName(baseName, &env, _control.data());

                if (const TemplateNameId *baseTemplId = baseName->asTemplateNameId()) {
                    // Another template that uses the dependent name.
                    // Ex.: template <class T> class A : public B<T> {};
                    if (baseTemplId->identifier() != templId->identifier()) {
                        if (LookupScopePrivate *nested = nestedType(baseName, origin))
                            baseBinding = nested->q;
                    }
                } else if (const QualifiedNameId *qBaseName = baseName->asQualifiedNameId()) {
                    // Qualified names in general.
                    // Ex.: template <class T> class A : public B<T>::Type {};
                    LookupScope *binding = q;
                    if (const Name *qualification = qBaseName->base()) {
                        const TemplateNameId *baseTemplName = qualification->asTemplateNameId();
                        if (!baseTemplName || !compareName(baseTemplName, templateSpecialization->name()))
                            binding = q->lookupType(qualification);
                    }
                    baseName = qBaseName->name();

                    if (binding)
                        baseBinding = binding->lookupType(baseName);
                }
            }

            if (baseBinding && !knownUsings.contains(baseBinding))
                instantiation->addUsing(baseBinding);
        }
        _alreadyConsideredTemplates.clear(templId);
        baseTemplateClassReference->_instantiations[templId] = instantiation;
        return instantiation;
    }

    if (allBases.isEmpty() || allBases.size() == knownUsings.size())
        return reference;

    // Find the missing bases for regular (non-template) types.
    // Ex.: class A : public B<Some>::Type {};
    foreach (const Name *baseName, allBases) {
        LookupScope *binding = q;
        if (const QualifiedNameId *qBaseName = baseName->asQualifiedNameId()) {
            if (const Name *qualification = qBaseName->base())
                binding = q->lookupType(qualification);
            else if (binding->parent() != 0)
                //if this is global identifier we take global namespace
                //Ex: class A{}; namespace NS { class A: public ::A{}; }
                binding = binding->d->globalNamespace();
            else
                //if we are in the global scope
                continue;
            baseName = qBaseName->name();
        }

        if (binding) {
            LookupScope * baseBinding = binding->lookupType(baseName);
            if (baseBinding && !knownUsings.contains(baseBinding))
                reference->addUsing(baseBinding);
        }
    }

    _alreadyConsideredClasses.clear(referenceClass);
    return reference;
}

LookupScopePrivate *Instantiator::instantiate(LookupScopePrivate *lookupScope,
                                              LookupScopePrivate *origin)
{
    lookupScope->flush();
    LookupScopePrivate *instantiation = lookupScope->allocateChild(lookupScope->_name);
    instantiation->_instantiationOrigin = origin;
    doInstantiate(lookupScope, instantiation);
    return instantiation;
}

void Instantiator::doInstantiate(LookupScopePrivate *lookupScope, LookupScopePrivate *instantiation)
{
    if (_alreadyConsideredInstantiations.contains(lookupScope))
        return;
    _alreadyConsideredInstantiations.insert(lookupScope);
    // The instantiation should have all symbols, enums, and usings from the reference.
    if (instantiation != lookupScope) {
        instantiation->_enums = lookupScope->_enums;
        auto typedefend = lookupScope->_typedefs.end();
        for (auto typedefit = lookupScope->_typedefs.begin();
             typedefit != typedefend;
             ++typedefit) {
            instantiation->_typedefs[typedefit->first] =
                    _cloner.symbol(typedefit->second, &_subst)->asDeclaration();
        }
        foreach (LookupScope *usingLookupScope, lookupScope->_usings)
            instantiation->_usings.append(instantiate(usingLookupScope->d, instantiation)->q);
        foreach (Symbol *s, lookupScope->_symbols) {
            Symbol *clone = _cloner.symbol(s, &_subst);
            if (!clone->enclosingScope()) // Not from the cache but just cloned.
                clone->setEnclosingScope(s->enclosingScope());
            instantiation->_symbols.append(clone);
            if (s == instantiation->_rootClass) {
                clone->setName(instantiation->_name);
                instantiation->_rootClass = clone->asClass();
            }
            if (Q_UNLIKELY(debug)) {
                Overview oo;
                oo.showFunctionSignatures = true;
                oo.showReturnTypes = true;
                oo.showTemplateParameters = true;
                qDebug() << "cloned" << oo(clone->type());
                if (Class *klass = clone->asClass()) {
                    const unsigned klassMemberCount = klass->memberCount();
                    for (unsigned i = 0; i < klassMemberCount; ++i){
                        Symbol *klassMemberAsSymbol = klass->memberAt(i);
                        if (klassMemberAsSymbol->isTypedef()) {
                            if (Declaration *declaration = klassMemberAsSymbol->asDeclaration())
                                qDebug() << "Member: " << oo(declaration->type(), declaration->name());
                        }
                    }
                }
            }
        }
    }
    auto cit = lookupScope->_nestedScopes.begin();
    for (; cit != lookupScope->_nestedScopes.end(); ++cit) {
        const Name *nestedName = cit->first;
        LookupScopePrivate *nestedLookupScope = cit->second;
        LookupScopePrivate *nestedInstantiation = instantiate(nestedLookupScope, instantiation);
        nestedInstantiation->_parent = instantiation;
        instantiation->_nestedScopes[nestedName] = nestedInstantiation;
    }
    _alreadyConsideredInstantiations.remove(lookupScope);
}

void LookupScopePrivate::flush()
{
    if (! _todo.isEmpty()) {
        const QList<Symbol *> todo = _todo;
        _todo.clear();

        foreach (Symbol *member, todo)
            _factory->process(member, q);
    }
}

void LookupScopePrivate::addSymbol(Symbol *symbol)
{
    _symbols.append(symbol);
}

void LookupScopePrivate::addTodo(Symbol *symbol)
{
    _todo.append(symbol);
}

void LookupScopePrivate::addUnscopedEnum(Enum *e)
{
    _enums.append(e);
}

void LookupScopePrivate::addTypedef(const Name *identifier, Declaration *d)
{
    _typedefs[identifier] = d;
}

void LookupScopePrivate::addUsing(LookupScope *u)
{
    _usings.append(u);
}

void LookupScopePrivate::addNestedType(const Name *alias, LookupScope *e)
{
    _nestedScopes[alias] = e->d;
}

LookupScope *LookupScopePrivate::findOrCreateType(
        const Name *name, LookupScopePrivate *origin, Class *clazz)
{
    if (! name)
        return q;
    if (! origin)
        origin = this;

    if (const QualifiedNameId *qName = name->asQualifiedNameId()) {
        if (! qName->base())
            return globalNamespace()->d->findOrCreateType(qName->name(), origin, clazz);

        return findOrCreateType(qName->base(), origin)->d->findOrCreateType(qName->name(), origin, clazz);

    } else if (name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId()) {
        LookupScopePrivate *e = nestedType(name, origin);

        if (! e) {
            e = allocateChild(name);
            e->_rootClass = clazz;
            _nestedScopes[name] = e;
        }

        return e->q;
    }

    return 0;
}

} // namespace Internal

CreateBindings::CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _control(QSharedPointer<Control>(new Control))
    , _expandTemplates(false)
{
    _globalNamespace = allocLookupScope(/*parent = */ 0, /*name = */ 0);
    _currentLookupScope = _globalNamespace;

    process(thisDocument);
}

CreateBindings::~CreateBindings()
{
    qDeleteAll(_entities);
}

LookupScope *CreateBindings::switchCurrentLookupScope(LookupScope *lookupScope)
{
    LookupScope *previous = _currentLookupScope;
    _currentLookupScope = lookupScope;
    return previous;
}

LookupScope *CreateBindings::globalNamespace() const
{
    return _globalNamespace;
}

LookupScope *CreateBindings::lookupType(Symbol *symbol, LookupScope *enclosingBinding)
{
    const QList<const Name *> path = LookupContext::path(symbol);
    return lookupType(path, enclosingBinding);
}

LookupScope *CreateBindings::lookupType(const QList<const Name *> &path,
                                             LookupScope *enclosingBinding)
{
    if (path.isEmpty())
        return _globalNamespace;

    if (enclosingBinding) {
        if (LookupScope *b = enclosingBinding->lookupType(path.last()))
            return b;
    }

    LookupScope *b = _globalNamespace->lookupType(path.at(0));

    for (int i = 1; b && i < path.size(); ++i)
        b = b->findType(path.at(i));

    return b;
}

void CreateBindings::process(Symbol *s, LookupScope *lookupScope)
{
    LookupScope *previous = switchCurrentLookupScope(lookupScope);
    accept(s);
    (void) switchCurrentLookupScope(previous);
}

void CreateBindings::process(Symbol *symbol)
{
    _currentLookupScope->d->addTodo(symbol);
}

LookupScope *CreateBindings::allocLookupScope(LookupScope *parent, const Name *name)
{
    LookupScope *e = new LookupScope(this, parent);
    e->d->_control = control();
    e->d->_name = name;
    _entities.append(e);
    return e;
}

void CreateBindings::process(Document::Ptr doc)
{
    if (! doc)
        return;

    if (Namespace *globalNamespace = doc->globalNamespace()) {
        if (! _processed.contains(globalNamespace)) {
            _processed.insert(globalNamespace);

            foreach (const Document::Include &i, doc->resolvedIncludes()) {
                if (Document::Ptr incl = _snapshot.document(i.resolvedFileName()))
                    process(incl);
            }

            accept(globalNamespace);
        }
    }
}

LookupScope *CreateBindings::enterLookupScopeBinding(Symbol *symbol)
{
    LookupScope *entity = _currentLookupScope->d->findOrCreateType(
                symbol->name(), 0, symbol->asClass());
    entity->d->addSymbol(symbol);

    return switchCurrentLookupScope(entity);
}

LookupScope *CreateBindings::enterGlobalLookupScope(Symbol *symbol)
{
    LookupScope *entity = _globalNamespace->d->findOrCreateType(
                symbol->name(), 0, symbol->asClass());
    entity->d->addSymbol(symbol);

    return switchCurrentLookupScope(entity);
}

bool CreateBindings::visit(Template *templ)
{
    if (Symbol *d = templ->declaration())
        accept(d);

    return false;
}

bool CreateBindings::visit(ExplicitInstantiation *inst)
{
    Q_UNUSED(inst);
    return false;
}

bool CreateBindings::visit(Namespace *ns)
{
    LookupScope *previous = enterLookupScopeBinding(ns);

    for (unsigned i = 0; i < ns->memberCount(); ++i)
        process(ns->memberAt(i));

    if (ns->isInline() && previous)
        previous->d->addUsing(_currentLookupScope);

    _currentLookupScope = previous;

    return false;
}

bool CreateBindings::visit(Class *klass)
{
    LookupScope *previous = _currentLookupScope;
    LookupScope *binding = 0;

    if (klass->name() && klass->name()->isQualifiedNameId())
        binding = _currentLookupScope->lookupType(klass->name());

    if (! binding)
        binding = _currentLookupScope->d->findOrCreateType(klass->name(), 0, klass);

    _currentLookupScope = binding;
    _currentLookupScope->d->addSymbol(klass);

    for (unsigned i = 0; i < klass->baseClassCount(); ++i)
        process(klass->baseClassAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(ForwardClassDeclaration *klass)
{
    if (! klass->isFriend()) {
        LookupScope *previous = enterLookupScopeBinding(klass);
        _currentLookupScope = previous;
    }

    return false;
}

bool CreateBindings::visit(Enum *e)
{
    if (e->isScoped()) {
        LookupScope *previous = enterLookupScopeBinding(e);
        _currentLookupScope = previous;
    } else {
        _currentLookupScope->d->addUnscopedEnum(e);
    }
    return false;
}

bool CreateBindings::visit(Declaration *decl)
{
    if (decl->isTypedef()) {
        _currentLookupScope->d->_hasTypedefs = true;
        FullySpecifiedType ty = decl->type();
        const Identifier *typedefId = decl->identifier();

        if (typedefId && ! (ty.isConst() || ty.isVolatile())) {
            if (ty->isNamedType()) {
                _currentLookupScope->d->addTypedef(typedefId, decl);
            } else if (Class *klass = ty->asClassType()) {
                if (const Identifier *nameId = decl->name()->asNameId()) {
                    LookupScope *binding
                        = _currentLookupScope->d->findOrCreateType(nameId, 0, klass);
                    binding->d->addSymbol(klass);
                }
            }
        }
    }
    if (Class *clazz = decl->type()->asClassType()) {
        if (const Name *name = clazz->name()) {
            if (const AnonymousNameId *anonymousNameId = name->asAnonymousNameId())
                _currentLookupScope->d->_declaredOrTypedefedAnonymouses.insert(anonymousNameId);
        }
    }
    return false;
}

bool CreateBindings::visit(Function *function)
{
    LookupScope *previous = _currentLookupScope;
    LookupScope *binding = lookupType(function, previous);
    if (!binding)
        return false;
    _currentLookupScope = binding;
    for (unsigned i = 0, count = function->memberCount(); i < count; ++i) {
        Symbol *s = function->memberAt(i);
        if (Block *b = s->asBlock())
            visit(b);
    }
    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(Block *block)
{
    LookupScope *previous = _currentLookupScope;

    LookupScope *binding = new LookupScope(this, previous);
    binding->d->_control = control();

    _currentLookupScope = binding;
    _currentLookupScope->d->addSymbol(block);

    for (unsigned i = 0; i < block->memberCount(); ++i)
        // we cannot use lazy processing here, because we have to know
        // does this block contain any other blocks or LookupScopes
        process(block->memberAt(i), _currentLookupScope);

    // we add this block to parent LookupScope only if it contains
    // any nested LookupScopes or other blocks(which have to contain
    // nested LookupScopes)
    if (! _currentLookupScope->d->_blocks.empty()
            || ! _currentLookupScope->d->_nestedScopes.empty()
            || ! _currentLookupScope->d->_enums.empty()
            || _currentLookupScope->d->_hasTypedefs
            || ! _currentLookupScope->d->_anonymouses.empty()) {
        previous->d->_blocks[block] = binding;
        _entities.append(binding);
    } else {
        delete binding;
        binding = 0;
    }

    _currentLookupScope = previous;

    return false;
}

bool CreateBindings::visit(BaseClass *b)
{
    if (LookupScope *base = _currentLookupScope->lookupType(b->name())) {
        _currentLookupScope->d->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo.prettyName(b->name());
    }
    return false;
}

bool CreateBindings::visit(UsingDeclaration *u)
{
    if (u->name()) {
        if (const QualifiedNameId *q = u->name()->asQualifiedNameId()) {
            if (const Identifier *unqualifiedId = q->name()->asNameId()) {
                if (LookupScope *delegate = _currentLookupScope->lookupType(q)) {
                    LookupScope *b = _currentLookupScope->d->findOrCreateType(unqualifiedId);
                    b->d->addUsing(delegate);
                }
            }
        }
    }
    return false;
}

bool CreateBindings::visit(UsingNamespaceDirective *u)
{
    if (LookupScope *e = _currentLookupScope->lookupType(u->name())) {
        _currentLookupScope->d->addUsing(e);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo.prettyName(u->name());
    }
    return false;
}

bool CreateBindings::visit(NamespaceAlias *a)
{
    if (! a->identifier()) {
        return false;

    } else if (LookupScope *e = _currentLookupScope->lookupType(a->namespaceName())) {
        if (a->name()->isNameId() || a->name()->isTemplateNameId() || a->name()->isAnonymousNameId())
            _currentLookupScope->d->addNestedType(a->name(), e);

    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo.prettyName(a->namespaceName());
    }

    return false;
}

bool CreateBindings::visit(ObjCClass *klass)
{
    LookupScope *previous = enterGlobalLookupScope(klass);

    process(klass->baseClass());

    for (unsigned i = 0; i < klass->protocolCount(); ++i)
        process(klass->protocolAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseClass *b)
{
    if (LookupScope *base = _globalNamespace->lookupType(b->name())) {
        _currentLookupScope->d->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo.prettyName(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardClassDeclaration *klass)
{
    LookupScope *previous = enterGlobalLookupScope(klass);
    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(ObjCProtocol *proto)
{
    LookupScope *previous = enterGlobalLookupScope(proto);

    for (unsigned i = 0; i < proto->protocolCount(); ++i)
        process(proto->protocolAt(i));

    for (unsigned i = 0; i < proto->memberCount(); ++i)
        process(proto->memberAt(i));

    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseProtocol *b)
{
    if (LookupScope *base = _globalNamespace->lookupType(b->name())) {
        _currentLookupScope->d->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo.prettyName(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardProtocolDeclaration *proto)
{
    LookupScope *previous = enterGlobalLookupScope(proto);
    _currentLookupScope = previous;
    return false;
}

bool CreateBindings::visit(ObjCMethod *)
{
    return false;
}

FullySpecifiedType CreateBindings::resolveTemplateArgument(Clone &cloner,
                                                           Subst &subst,
                                                           LookupScope *origin,
                                                           const Template *specialization,
                                                           const TemplateNameId *instantiation,
                                                           unsigned index)
{
    FullySpecifiedType ty;
    CPP_ASSERT(specialization && instantiation, return ty);

    const TypenameArgument *tParam = 0;
    if (Symbol *tArgument = specialization->templateParameterAt(index))
        tParam = tArgument->asTypenameArgument();
    if (!tParam)
        return ty;

    if (index < instantiation->templateArgumentCount())
        ty = instantiation->templateArgumentAt(index);
    else
        ty = cloner.type(tParam->type(), &subst);

    TypeResolver typeResolver(*this);
    Scope *resolveScope = specialization->enclosingScope();
    typeResolver.resolve(&ty, &resolveScope, origin);
    const TemplateNameId *templSpecId = specialization->name()->asTemplateNameId();
    const unsigned templSpecArgumentCount = templSpecId ? templSpecId->templateArgumentCount() : 0;
    if (index < templSpecArgumentCount && templSpecId->templateArgumentAt(index)->isPointerType()) {
        if (PointerType *pointerType = ty->asPointerType())
            ty = pointerType->elementType();
    }

    if (const Name *name = tParam->name())
        subst.bind(cloner.name(name, &subst), ty);
    return ty;
}

void CreateBindings::initializeSubst(Clone &cloner,
                                     Subst &subst,
                                     LookupScope *origin,
                                     const Template *specialization,
                                     const TemplateNameId *instantiation)
{
    const unsigned argumentCountOfSpecialization = specialization->templateParameterCount();

    for (unsigned i = 0; i < argumentCountOfSpecialization; ++i)
        resolveTemplateArgument(cloner, subst, origin, specialization, instantiation, i);
}

} // namespace CPlusPlus
