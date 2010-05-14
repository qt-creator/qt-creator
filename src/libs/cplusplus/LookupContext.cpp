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

#include "LookupContext.h"
#include "ResolveExpression.h"
#include "Overview.h"
#include "CppBindings.h"
#include "GenTemplateInstance.h"

#include <CoreTypes.h>
#include <Symbols.h>
#include <Literals.h>
#include <Names.h>
#include <Scope.h>
#include <Control.h>

#include <QtDebug>

namespace {
    const bool debug = ! qgetenv("CPLUSPLUS_LOOKUPCONTEXT_DEBUG").isEmpty();
}

using namespace CPlusPlus;

static void fullyQualifiedName_helper(Symbol *symbol, QList<const Name *> *names)
{
    if (! symbol)
        return;

    fullyQualifiedName_helper(symbol->enclosingSymbol(), names);

    if (symbol->name()) {
        if (symbol->isClass() || symbol->isNamespace()) {
            if (const QualifiedNameId *q = symbol->name()->asQualifiedNameId()) {
                for (unsigned i = 0; i < q->nameCount(); ++i)
                    names->append(q->nameAt(i));

            } else if (symbol->name()->isNameId() || symbol->name()->isTemplateNameId()) {
                names->append(symbol->name());

            }

        } else if (symbol->isFunction()) {
            if (const QualifiedNameId *q = symbol->name()->asQualifiedNameId()) {
                for (unsigned i = 0; i < q->nameCount() - 1; ++i)
                    names->append(q->nameAt(i));
            }
        }
    }

}

bool ClassOrNamespace::CompareName::operator()(const Name *name, const Name *other) const
{
    Q_ASSERT(name != 0);
    Q_ASSERT(other != 0);

    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();
    return strcmp(id->chars(), otherId->chars()) < 0;
}

/////////////////////////////////////////////////////////////////////
// LookupContext
/////////////////////////////////////////////////////////////////////
LookupContext::LookupContext()
    : _control(0)
{ }

LookupContext::LookupContext(Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _expressionDocument(Document::create("<LookupContext>")),
      _thisDocument(thisDocument),
      _snapshot(snapshot)
{
    _control = _expressionDocument->control();
}

LookupContext::LookupContext(Document::Ptr expressionDocument,
                             Document::Ptr thisDocument,
                             const Snapshot &snapshot)
    : _expressionDocument(expressionDocument),
      _thisDocument(thisDocument),
      _snapshot(snapshot)
{
    _control = _expressionDocument->control();
}

LookupContext::LookupContext(const LookupContext &other)
    : _control(other._control),
      _expressionDocument(other._expressionDocument),
      _thisDocument(other._thisDocument),
      _snapshot(other._snapshot),
      _bindings(other._bindings)
{ }

LookupContext &LookupContext::operator = (const LookupContext &other)
{
    _control = other._control;
    _expressionDocument = other._expressionDocument;
    _thisDocument = other._thisDocument;
    _snapshot = other._snapshot;
    _bindings = other._bindings;
    return *this;
}

QList<const Name *> LookupContext::fullyQualifiedName(Symbol *symbol)
{
    QList<const Name *> names;
    fullyQualifiedName_helper(symbol, &names);
    return names;
}

QSharedPointer<CreateBindings> LookupContext::bindings() const
{
    if (! _bindings)
        _bindings = QSharedPointer<CreateBindings>(new CreateBindings(_thisDocument, _snapshot));

    return _bindings;
}

void LookupContext::setBindings(QSharedPointer<CreateBindings> bindings)
{
    _bindings = bindings;
}

bool LookupContext::isValid() const
{ return _control != 0; }

Control *LookupContext::control() const
{ return _control; }

Document::Ptr LookupContext::expressionDocument() const
{ return _expressionDocument; }

Document::Ptr LookupContext::thisDocument() const
{ return _thisDocument; }

Document::Ptr LookupContext::document(const QString &fileName) const
{ return _snapshot.document(fileName); }

Snapshot LookupContext::snapshot() const
{ return _snapshot; }

ClassOrNamespace *LookupContext::globalNamespace() const
{
    return bindings()->globalNamespace();
}

ClassOrNamespace *LookupContext::lookupType(const Name *name, Scope *scope) const
{
    if (ClassOrNamespace *b = bindings()->lookupType(scope->owner()))
        return b->lookupType(name);

    return 0;
}

ClassOrNamespace *LookupContext::lookupType(Symbol *symbol) const
{
    return bindings()->lookupType(symbol);
}

QList<Symbol *> LookupContext::lookup(const Name *name, Scope *scope) const
{
    QList<Symbol *> candidates;

    if (! name)
        return candidates;

    const Identifier *id = name->identifier();

    for (; scope; scope = scope->enclosingScope()) {
        if (id && scope->isBlockScope()) {
            bindings()->lookupInScope(name, scope, &candidates, /*templateId = */ 0);

            if (! candidates.isEmpty())
                break; // it's a local.

            for (unsigned index = 0; index < scope->symbolCount(); ++index) {
                Symbol *member = scope->symbolAt(index);

                if (UsingNamespaceDirective *u = member->asUsingNamespaceDirective()) {
                    if (Namespace *enclosingNamespace = u->enclosingNamespaceScope()->owner()->asNamespace()) {
                        if (ClassOrNamespace *b = bindings()->lookupType(enclosingNamespace)) {
                            if (ClassOrNamespace *uu = b->lookupType(u->name())) {
                                candidates = uu->lookup(name);

                                if (! candidates.isEmpty())
                                    return candidates;
                            }
                        }
                    }
                }
            }

        } else if (scope->isFunctionScope()) {
            Function *fun = scope->owner()->asFunction();
            bindings()->lookupInScope(name, fun->arguments(), &candidates, /*templateId = */ 0);
            if (! candidates.isEmpty())
                break; // it's a formal argument.

            if (fun->name() && fun->name()->isQualifiedNameId()) {
                if (ClassOrNamespace *binding = bindings()->lookupType(fun))
                    return binding->lookup(name);
            }

        } else if (scope->isObjCMethodScope()) {
            ObjCMethod *method = scope->owner()->asObjCMethod();
            bindings()->lookupInScope(name, method->arguments(), &candidates, /*templateId = */ 0);
            if (! candidates.isEmpty())
                break; // it's a formal argument.

        } else if (scope->isClassScope() || scope->isNamespaceScope()
                    || scope->isObjCClassScope() || scope->isObjCProtocolScope()) {
            if (ClassOrNamespace *binding = bindings()->lookupType(scope->owner()))
                return binding->lookup(name);

            break;
        }
    }

    return candidates;
}

ClassOrNamespace::ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent)
    : _factory(factory), _parent(parent), _templateId(0)
{
}

const TemplateNameId *ClassOrNamespace::templateId() const
{
    return _templateId;
}

ClassOrNamespace *ClassOrNamespace::parent() const
{
    return _parent;
}

QList<ClassOrNamespace *> ClassOrNamespace::usings() const
{
    const_cast<ClassOrNamespace *>(this)->flush();
    return _usings;
}

QList<Enum *> ClassOrNamespace::enums() const
{
    const_cast<ClassOrNamespace *>(this)->flush();
    return _enums;
}

QList<Symbol *> ClassOrNamespace::symbols() const
{
    if (_templateId && ! _usings.isEmpty())
        return _usings.first()->symbols(); // ask to the base implementation

    const_cast<ClassOrNamespace *>(this)->flush();
    return _symbols;
}

ClassOrNamespace *ClassOrNamespace::globalNamespace() const
{
    ClassOrNamespace *e = const_cast<ClassOrNamespace *>(this);

    do {
        if (! e->_parent)
            break;

        e = e->_parent;
    } while (e);

    return e;
}

QList<Symbol *> ClassOrNamespace::find(const Name *name)
{
    return lookup_helper(name, false);
}

QList<Symbol *> ClassOrNamespace::lookup(const Name *name)
{
    return lookup_helper(name, true);
}

QList<Symbol *> ClassOrNamespace::lookup_helper(const Name *name, bool searchInEnclosingScope)
{
    QList<Symbol *> result;

    if (name) {
        if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            ClassOrNamespace *binding = this;

            if (q->isGlobal())
                binding = globalNamespace();

            if (q->nameCount() == 1)
                return binding->find(q->unqualifiedNameId());

            binding = binding->lookupType(q->nameAt(0));

            for (unsigned index = 1; binding && index < q->nameCount() - 1; ++index)
                binding = binding->findType(q->nameAt(index));

            if (binding)
                result = binding->find(q->unqualifiedNameId());

            return result;
        }

        QSet<ClassOrNamespace *> processed;
        ClassOrNamespace *binding = this;
        do {
            lookup_helper(name, binding, &result, &processed, /*templateId = */ 0);
            binding = binding->_parent;
        } while (searchInEnclosingScope && binding);
    }

    return result;
}

void ClassOrNamespace::lookup_helper(const Name *name, ClassOrNamespace *binding,
                                     QList<Symbol *> *result,
                                     QSet<ClassOrNamespace *> *processed,
                                     const TemplateNameId *templateId)
{
    if (binding && ! processed->contains(binding)) {
        processed->insert(binding);

        foreach (Symbol *s, binding->symbols()) {
            if (ScopedSymbol *scoped = s->asScopedSymbol())
                _factory->lookupInScope(name, scoped->members(), result, templateId);
        }

        foreach (Enum *e, binding->enums())
            _factory->lookupInScope(name, e->members(), result, templateId);

        foreach (ClassOrNamespace *u, binding->usings())
            lookup_helper(name, u, result, processed, binding->_templateId);
    }
}

void CreateBindings::lookupInScope(const Name *name, Scope *scope,
                                   QList<Symbol *> *result,
                                   const TemplateNameId *templateId)
{
    Q_UNUSED(templateId);

    if (! name) {
        return;

    } else if (const OperatorNameId *op = name->asOperatorNameId()) {
        for (Symbol *s = scope->lookat(op->kind()); s; s = s->next()) {
            if (! s->name())
                continue;
            else if (! s->name()->isEqualTo(op))
                continue;

            result->append(s);
        }

    } else if (const Identifier *id = name->identifier()) {
        for (Symbol *s = scope->lookat(id); s; s = s->next()) {
            if (! id->isEqualTo(s->identifier()))
                continue;
            else if (s->name()->isQualifiedNameId())
                continue; // skip qualified ids.

#if 1
            if (templateId && (s->isDeclaration() || s->isFunction())) {

                FullySpecifiedType ty = GenTemplateInstance::instantiate(templateId, s, _control);

                Overview oo;
                oo.setShowFunctionSignatures(true);
                oo.setShowReturnTypes(true);

                qDebug() << "TODO: instantiate:" << oo(s->type(), s->name()) << "using:" << oo(templateId)
                        << oo(ty);

#if 0
                if (Declaration *decl = s->asDeclaration()) {
                    qDebug() << "instantiate declaration";
                    Declaration *d = _control->newDeclaration(0, 0);
                    d->copy(decl);
                    d->setType(ty);
                    result->append(d);
                    continue;
                } else if (Function *fun = s->asFunction()) {
                    qDebug() << "instantiate function";
                    Function *d = ty->asFunctionType();
                    d->copy(fun);
                    result->append(d);
                    continue;
                }
#endif

            }
#endif

            result->append(s);
        }

    }
}

ClassOrNamespace *ClassOrNamespace::lookupType(const Name *name)
{
    if (! name)
        return 0;

    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ true);
}

ClassOrNamespace *ClassOrNamespace::findType(const Name *name)
{
    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ false);
}

ClassOrNamespace *ClassOrNamespace::lookupType_helper(const Name *name,
                                                      QSet<ClassOrNamespace *> *processed,
                                                      bool searchInEnclosingScope)
{
    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        ClassOrNamespace *e = this;

        if (q->isGlobal())
            e = globalNamespace();

        e = e->lookupType(q->nameAt(0));

        for (unsigned index = 1; e && index < q->nameCount(); ++index)
            e = e->findType(q->nameAt(index));

        return e;

    } else if (! processed->contains(this)) {
        processed->insert(this);

        if (name->isNameId() || name->isTemplateNameId()) {
            flush();

            if (ClassOrNamespace *e = nestedType(name))
                return e;

            foreach (ClassOrNamespace *u, usings()) {
                if (ClassOrNamespace *r = u->lookupType_helper(name, processed, /*searchInEnclosingScope =*/ false))
                    return r;
            }
        }

        if (_parent && searchInEnclosingScope)
            return _parent->lookupType_helper(name, processed, searchInEnclosingScope);
    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::nestedType(const Name *name) const
{
    Q_ASSERT(name != 0);
    Q_ASSERT(name->isNameId() || name->isTemplateNameId());

    const_cast<ClassOrNamespace *>(this)->flush();

    Table::const_iterator it = _classOrNamespaces.find(name);

    if (it == _classOrNamespaces.end())
        return 0;

    ClassOrNamespace *c = it->second;

    if (const TemplateNameId *templId = name->asTemplateNameId()) {
        foreach (ClassOrNamespace *i, c->_instantiations) {
            if (templId->isEqualTo(i->_templateId))
                return i;
        }

        ClassOrNamespace *i = _factory->allocClassOrNamespace(c);
        i->_templateId = templId;
        i->_usings.append(c);
        c->_instantiations.append(i);
        return i;
    }

    return c;
}

void ClassOrNamespace::flush()
{
    if (! _todo.isEmpty()) {
        const QList<Symbol *> todo = _todo;
        _todo.clear();

        foreach (Symbol *member, todo)
            _factory->process(member, this);
    }
}

void ClassOrNamespace::addSymbol(Symbol *symbol)
{
    _symbols.append(symbol);
}

void ClassOrNamespace::addTodo(Symbol *symbol)
{
    _todo.append(symbol);
}

void ClassOrNamespace::addEnum(Enum *e)
{
    _enums.append(e);
}

void ClassOrNamespace::addUsing(ClassOrNamespace *u)
{
    _usings.append(u);
}

void ClassOrNamespace::addNestedType(const Name *alias, ClassOrNamespace *e)
{
    _classOrNamespaces[alias] = e;
}

ClassOrNamespace *ClassOrNamespace::findOrCreateType(const Name *name)
{
    if (! name)
        return this;

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        ClassOrNamespace *e = this;

        for (unsigned i = 0; e && i < q->nameCount(); ++i)
            e = e->findOrCreateType(q->nameAt(i));

        return e;

    } else if (name->isNameId() || name->isTemplateNameId()) {
        ClassOrNamespace *e = nestedType(name);

        if (! e) {
            e = _factory->allocClassOrNamespace(this);
            _classOrNamespaces[name] = e;
        }

        return e;
    }

    return 0;
}

CreateBindings::CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot)
    : _snapshot(snapshot)
{
    _control = new Control();
    _globalNamespace = allocClassOrNamespace(/*parent = */ 0);
    _currentClassOrNamespace = _globalNamespace;

    process(thisDocument);
}

CreateBindings::~CreateBindings()
{
    qDeleteAll(_entities);
    delete _control;
}

ClassOrNamespace *CreateBindings::switchCurrentClassOrNamespace(ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    _currentClassOrNamespace = classOrNamespace;
    return previous;
}

ClassOrNamespace *CreateBindings::globalNamespace() const
{
    return _globalNamespace;
}

ClassOrNamespace *CreateBindings::lookupType(Symbol *symbol)
{
    const QList<const Name *> names = LookupContext::fullyQualifiedName(symbol);

    if (names.isEmpty())
        return _globalNamespace;

    ClassOrNamespace *b = _globalNamespace->lookupType(names.at(0));

    for (int i = 1; b && i < names.size(); ++i)
        b = b->findType(names.at(i));

    return b;
}

void CreateBindings::process(Symbol *s, ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = switchCurrentClassOrNamespace(classOrNamespace);
    accept(s);
    (void) switchCurrentClassOrNamespace(previous);
}

void CreateBindings::process(Symbol *symbol)
{
    _currentClassOrNamespace->addTodo(symbol);
}

Control *CreateBindings::control() const
{
    return _control;
}

ClassOrNamespace *CreateBindings::allocClassOrNamespace(ClassOrNamespace *parent)
{
    ClassOrNamespace *e = new ClassOrNamespace(this, parent);
    _entities.append(e);
    return e;
}

void CreateBindings::process(Document::Ptr doc)
{
    if (! doc)
        return;

    else if (Namespace *globalNamespace = doc->globalNamespace()) {
        if (! _processed.contains(globalNamespace)) {
            _processed.insert(globalNamespace);

            foreach (const Document::Include &i, doc->includes()) {
                if (Document::Ptr incl = _snapshot.document(i.fileName()))
                    process(incl);
            }

            accept(globalNamespace);
        }
    }
}

ClassOrNamespace *CreateBindings::enterClassOrNamespaceBinding(Symbol *symbol)
{
    ClassOrNamespace *entity = _currentClassOrNamespace->findOrCreateType(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

ClassOrNamespace *CreateBindings::enterGlobalClassOrNamespace(Symbol *symbol)
{
    ClassOrNamespace *entity = _globalNamespace->findOrCreateType(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

bool CreateBindings::visit(Namespace *ns)
{
    ClassOrNamespace *previous = enterClassOrNamespaceBinding(ns);

    for (unsigned i = 0; i < ns->memberCount(); ++i)
        process(ns->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(Class *klass)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    ClassOrNamespace *binding = 0;

    if (klass->name() && klass->name()->isQualifiedNameId())
        binding = _currentClassOrNamespace->lookupType(klass->name());

    if (! binding)
        binding = _currentClassOrNamespace->findOrCreateType(klass->name());

    _currentClassOrNamespace = binding;
    _currentClassOrNamespace->addSymbol(klass);

    for (unsigned i = 0; i < klass->baseClassCount(); ++i)
        process(klass->baseClassAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ForwardClassDeclaration *klass)
{
    if (! klass->isFriend()) {
        ClassOrNamespace *previous = enterClassOrNamespaceBinding(klass);
        _currentClassOrNamespace = previous;
    }

    return false;
}

bool CreateBindings::visit(Enum *e)
{
    _currentClassOrNamespace->addEnum(e);
    return false;
}

bool CreateBindings::visit(Declaration *decl)
{
    if (decl->isTypedef()) {
        const FullySpecifiedType ty = decl->type();
        const Identifier *typedefId = decl->identifier();

        if (typedefId && ! (ty.isConst() || ty.isVolatile())) {
            if (const NamedType *namedTy = ty->asNamedType()) {
                if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(namedTy->name())) {
                    _currentClassOrNamespace->addNestedType(decl->name(), e);
                } else if (false) {
                    Overview oo;
                    qDebug() << "found entity not found for" << oo(namedTy->name());
                }
            }
        }
    }

    return false;
}

bool CreateBindings::visit(Function *)
{
    return false;
}

bool CreateBindings::visit(BaseClass *b)
{
    if (ClassOrNamespace *base = _currentClassOrNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(UsingNamespaceDirective *u)
{
    if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(u->name())) {
        _currentClassOrNamespace->addUsing(e);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo(u->name());
    }
    return false;
}

bool CreateBindings::visit(NamespaceAlias *a)
{
    if (! a->identifier()) {
        return false;

    } else if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(a->namespaceName())) {
        if (a->name()->isNameId() || a->name()->isTemplateNameId())
            _currentClassOrNamespace->addNestedType(a->name(), e);

    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo(a->namespaceName());
    }

    return false;
}

bool CreateBindings::visit(ObjCClass *klass)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(klass);

    process(klass->baseClass());

    for (unsigned i = 0; i < klass->protocolCount(); ++i)
        process(klass->protocolAt(i));

    for (unsigned i = 0; i < klass->memberCount(); ++i)
        process(klass->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseClass *b)
{
    if (ClassOrNamespace *base = _globalNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardClassDeclaration *klass)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(klass);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCProtocol *proto)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(proto);

    for (unsigned i = 0; i < proto->protocolCount(); ++i)
        process(proto->protocolAt(i));

    for (unsigned i = 0; i < proto->memberCount(); ++i)
        process(proto->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseProtocol *b)
{
    if (ClassOrNamespace *base = _globalNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardProtocolDeclaration *proto)
{
    ClassOrNamespace *previous = enterGlobalClassOrNamespace(proto);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCMethod *)
{
    return false;
}
