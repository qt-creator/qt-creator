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
#include <cxxabi.h>

//#define CPLUSPLUS_NO_LAZY_LOOKUP

using namespace CPlusPlus;

static void fullyQualifiedName(Symbol *symbol, QList<const Name *> *names)
{
    if (! symbol)
        return;

    fullyQualifiedName(symbol->enclosingSymbol(), names);

    if (symbol->name() && (symbol->isClass() || symbol->isNamespace())) {
        if (const QualifiedNameId *q = symbol->name()->asQualifiedNameId()) {
            for (unsigned i = 0; i < q->nameCount(); ++i)
                names->append(q->nameAt(i));

        } else if (symbol->name()->isNameId() || symbol->name()->isTemplateNameId()) {
            names->append(symbol->name());

        }
    }
}

bool ClassOrNamespace::CompareName::operator()(const Name *name, const Name *other) const
{
    Q_ASSERT(name != 0);
    Q_ASSERT(other != 0);
    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();
    return std::lexicographical_compare(id->begin(), id->end(), otherId->begin(), otherId->end());
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

ClassOrNamespace *LookupContext::classOrNamespace(const Name *name, Scope *scope) const
{
    if (ClassOrNamespace *b = bindings()->findClassOrNamespace(scope->owner()))
        return b->lookupClassOrNamespace(name);

    return 0;
}

ClassOrNamespace *LookupContext::classOrNamespace(Symbol *symbol) const
{
    return bindings()->findClassOrNamespace(symbol);
}

ClassOrNamespace *LookupContext::classOrNamespace(const Name *name, Symbol *lastVisibleSymbol) const
{
    Scope *scope = _thisDocument->globalSymbols();

    if (lastVisibleSymbol && lastVisibleSymbol->scope())
        scope = lastVisibleSymbol->scope();

    return classOrNamespace(name, scope);
}

QList<Symbol *> LookupContext::lookup(const Name *name, Symbol *lastVisibleSymbol) const
{
    Scope *scope = _thisDocument->globalSymbols();

    if (lastVisibleSymbol && lastVisibleSymbol->scope())
        scope = lastVisibleSymbol->scope();

    return lookup(name, scope);
}

QList<Symbol *> LookupContext::lookup(const Name *name, Scope *scope) const
{
    QList<Symbol *> candidates;

    if (! name)
        return candidates;

    const Identifier *id = name->identifier();

    for (; scope; scope = scope->enclosingScope()) {
        if (id && scope->isBlockScope()) {
            bindings()->lookup_helper(name, scope, &candidates, /*templateId = */ 0);

            if (! candidates.isEmpty())
                break; // it's a local.

            for (unsigned index = 0; index < scope->symbolCount(); ++index) {
                Symbol *member = scope->symbolAt(index);

                if (UsingNamespaceDirective *u = member->asUsingNamespaceDirective()) {
                    if (Namespace *enclosingNamespace = u->enclosingNamespaceScope()->owner()->asNamespace()) {
                        if (ClassOrNamespace *b = bindings()->findClassOrNamespace(enclosingNamespace)) {
                            if (ClassOrNamespace *uu = b->lookupClassOrNamespace(u->name())) {
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
            bindings()->lookup_helper(name, fun->arguments(), &candidates, /*templateId = */ 0);
            if (! candidates.isEmpty())
                break; // it's a formal argument.

            if (fun->name() && fun->name()->isQualifiedNameId()) {
                const QualifiedNameId *q = fun->name()->asQualifiedNameId();

                QList<const Name *> path;
                fullyQualifiedName(scope->owner(), &path);

                for (unsigned index = 0; index < q->nameCount() - 1; ++index) { // ### TODO remove me.
                    const Name *name = q->nameAt(index);

                    if (name->isNameId() || name->isTemplateNameId())
                        path.append(name);
                }

                if (ClassOrNamespace *binding = bindings()->findClassOrNamespace(path))
                    return binding->lookup(name);
            }

        } else if (scope->isObjCMethodScope()) {
            ObjCMethod *method = scope->owner()->asObjCMethod();
            bindings()->lookup_helper(name, method->arguments(), &candidates, /*templateId = */ 0);
            if (! candidates.isEmpty())
                break; // it's a formal argument.

        } else if (scope->isClassScope() || scope->isNamespaceScope()
                    || scope->isObjCClassScope() || scope->isObjCProtocolScope()) {
            if (ClassOrNamespace *binding = bindings()->findClassOrNamespace(scope->owner()))
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

QList<Symbol *> ClassOrNamespace::lookup(const Name *name)
{
    QList<Symbol *> result;
    if (! name)
        return result;

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        ClassOrNamespace *binding = this;

        if (q->isGlobal())
            binding = globalNamespace();

        if (q->nameCount() == 1)
            return binding->lookup(q->unqualifiedNameId());

        binding = binding->lookupClassOrNamespace(q->nameAt(0));

        for (unsigned index = 1; binding && index < q->nameCount() - 1; ++index)
            binding = binding->findClassOrNamespace(q->nameAt(index));

        if (binding)
            result = binding->lookup(q->unqualifiedNameId());

        return result;
    }

    QSet<ClassOrNamespace *> processed;
    ClassOrNamespace *binding = this;
    do {
        lookup_helper(name, binding, &result, &processed, /*templateId = */ 0);
        binding = binding->_parent;
    } while (binding);

    return result;
}

void ClassOrNamespace::lookup_helper(const Name *name, ClassOrNamespace *binding,
                                     QList<Symbol *> *result,
                                     QSet<ClassOrNamespace *> *processed,
                                     const TemplateNameId *templateId)
{
    if (! binding)
        return;

    else if (! processed->contains(binding)) {
        processed->insert(binding);

        //Overview oo;
        //qDebug() << "search for:" << oo(name) << "template:" << oo(templateId) << "b:" << oo(binding->_templateId);

        foreach (Symbol *s, binding->symbols()) {
            if (ScopedSymbol *scoped = s->asScopedSymbol())
                _factory->lookup_helper(name, scoped->members(), result, templateId);
        }

        foreach (Enum *e, binding->enums())
            _factory->lookup_helper(name, e->members(), result, templateId);

        foreach (ClassOrNamespace *u, binding->usings())
            lookup_helper(name, u, result, processed, binding->_templateId);

        //qDebug() << "=======" << oo(name) << "template:" << oo(binding->_templateId);
    }
}

void CreateBindings::lookup_helper(const Name *name, Scope *scope,
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
            if (! s->name())
                continue;
            else if (! id->isEqualTo(s->identifier()))
                continue;
            else if (s->name()->isQualifiedNameId()) {
#if 0
                Overview oo;
                oo.setShowReturnTypes(true);
                oo.setShowFunctionSignatures(true);
                qDebug() << "SKIP:" << oo(s->type(), s->name()) << s->fileName() << s->line() << s->column();
#endif
                continue;
            }

#if 0
            if (templateId && (s->isDeclaration() || s->isFunction())) {

                FullySpecifiedType ty = GenTemplateInstance::instantiate(templateId, s, control);

                Overview oo;
                oo.setShowFunctionSignatures(true);
                oo.setShowReturnTypes(true);

                qDebug() << "TODO: instantiate:" << oo(s->type(), s->name()) << "using:" << oo(templateId)
                        << oo(ty);

                if (Declaration *decl = s->asDeclaration()) {
                    qDebug() << "instantiate declaration";
                    qDebug() << "is typedef:" << ty.isTypedef() << s->isTypedef() << s->type().isTypedef();
                    Declaration *d = control->newDeclaration(0, 0);
                    d->setStorage(decl->storage());
                    d->setName(decl->name());
                    d->setType(ty);
                    d->setScope(decl->scope());
                    result->append(d);
                    continue;
                } else if (Function *fun = s->asFunction()) {
                    qDebug() << "instantiate function";
                    Function *d = ty->asFunctionType();
                    d->setStorage(fun->storage());
                    d->setScope(fun->scope());
                    result->append(d);
                    continue;
                }
            }
#endif

            result->append(s);
        }

    }
}

ClassOrNamespace *ClassOrNamespace::lookupClassOrNamespace(const Name *name)
{
    if (! name)
        return 0;

    QSet<ClassOrNamespace *> processed;
    return lookupClassOrNamespace_helper(name, &processed);
}

ClassOrNamespace *ClassOrNamespace::findClassOrNamespace(const Name *name)
{
    QSet<ClassOrNamespace *> processed;
    return findClassOrNamespace_helper(name, &processed);
}

ClassOrNamespace *ClassOrNamespace::findClassOrNamespace(const QList<const Name *> &path)
{
    if (path.isEmpty())
        return globalNamespace();

    ClassOrNamespace *e = this;

    for (int i = 0; e && i < path.size(); ++i) {
        QSet<ClassOrNamespace *> processed;
        e = e->findClassOrNamespace_helper(path.at(i), &processed);
    }

    return e;
}

ClassOrNamespace *ClassOrNamespace::lookupClassOrNamespace_helper(const Name *name,
                                                                  QSet<ClassOrNamespace *> *processed)
{
    Q_ASSERT(name != 0);

    if (! processed->contains(this)) {
        processed->insert(this);

        if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            ClassOrNamespace *e = this;

            if (q->isGlobal())
                e = globalNamespace();

            e = e->lookupClassOrNamespace(q->nameAt(0));

            for (unsigned index = 1; e && index < q->nameCount(); ++index) {
                QSet<ClassOrNamespace *> processed;
                e = e->findClassOrNamespace_helper(q->nameAt(index), &processed);
            }

            return e;

        } else if (name->isNameId() || name->isTemplateNameId()) {
            if (ClassOrNamespace *e = nestedClassOrNamespace(name))
                return e;

            foreach (ClassOrNamespace *u, usings()) {
                if (ClassOrNamespace *r = u->lookupClassOrNamespace_helper(name, processed))
                    return r;
            }
        }

        if (_parent)
            return _parent->lookupClassOrNamespace_helper(name, processed);
    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::findClassOrNamespace_helper(const Name *name,
                                                                QSet<ClassOrNamespace *> *processed)
{
    if (! name) {
        return 0;

    } else if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        ClassOrNamespace *e = this;

        if (q->isGlobal())
            e = globalNamespace();

        for (unsigned i = 0; e && i < q->nameCount(); ++i) {
            QSet<ClassOrNamespace *> processed;
            e = e->findClassOrNamespace_helper(q->nameAt(i), &processed);
        }

        return e;

    } else if (name->isNameId() || name->isTemplateNameId()) {
        if (ClassOrNamespace *e = nestedClassOrNamespace(name))
            return e;

        else if (! processed->contains(this)) {
            processed->insert(this);

            foreach (ClassOrNamespace *u, usings()) {
                if (ClassOrNamespace *e = u->findClassOrNamespace_helper(name, processed))
                    return e;
            }
        }

    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::nestedClassOrNamespace(const Name *name) const
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
#ifndef CPLUSPLUS_NO_LAZY_LOOKUP
    if (! _todo.isEmpty()) {
        const QList<Symbol *> todo = _todo;
        _todo.clear();

        foreach (Symbol *member, todo)
            _factory->process(member, this);
    }
#endif
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

void ClassOrNamespace::addNestedClassOrNamespace(const Name *alias, ClassOrNamespace *e)
{
    _classOrNamespaces[alias] = e;
}

ClassOrNamespace *ClassOrNamespace::findOrCreate(const Name *name)
{
    if (! name)
        return this;

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        ClassOrNamespace *e = this;

        for (unsigned i = 0; e && i < q->nameCount(); ++i)
            e = e->findOrCreate(q->nameAt(i));

        return e;

    } else if (name->isNameId() || name->isTemplateNameId()) {
        ClassOrNamespace *e = nestedClassOrNamespace(name);

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

ClassOrNamespace *CreateBindings::switchCurrentEntity(ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    _currentClassOrNamespace = classOrNamespace;
    return previous;
}

ClassOrNamespace *CreateBindings::globalNamespace() const
{
    return _globalNamespace;
}

ClassOrNamespace *CreateBindings::findClassOrNamespace(Symbol *symbol)
{
    QList<const Name *> names;
    fullyQualifiedName(symbol, &names);

    if (names.isEmpty())
        return _globalNamespace;

    ClassOrNamespace *b = _globalNamespace->lookupClassOrNamespace(names.at(0));

    for (int i = 1; b && i < names.size(); ++i)
        b = b->findClassOrNamespace(names.at(i));

    return b;
}

ClassOrNamespace *CreateBindings::findClassOrNamespace(const QList<const Name *> &path)
{
    ClassOrNamespace *e = _globalNamespace->findClassOrNamespace(path);
    return e;
}

void CreateBindings::process(Symbol *s, ClassOrNamespace *classOrNamespace)
{
    ClassOrNamespace *previous = switchCurrentEntity(classOrNamespace);
    accept(s);
    (void) switchCurrentEntity(previous);
}

void CreateBindings::process(Symbol *symbol)
{
#ifndef CPLUSPLUS_NO_LAZY_LOOKUP
    _currentClassOrNamespace->addTodo(symbol);
#else
    accept(symbol);
#endif
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

ClassOrNamespace *CreateBindings::enterEntity(Symbol *symbol)
{
    ClassOrNamespace *entity = _currentClassOrNamespace->findOrCreate(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentEntity(entity);
}

ClassOrNamespace *CreateBindings::enterGlobalEntity(Symbol *symbol)
{
    ClassOrNamespace *entity = _globalNamespace->findOrCreate(symbol->name());
    entity->addSymbol(symbol);

    return switchCurrentEntity(entity);
}

bool CreateBindings::visit(Namespace *ns)
{
    ClassOrNamespace *previous = enterEntity(ns);

    for (unsigned i = 0; i < ns->memberCount(); ++i)
        process(ns->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(Class *klass)
{
    ClassOrNamespace *previous = enterEntity(klass);

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
        ClassOrNamespace *previous = enterEntity(klass);
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
                if (ClassOrNamespace *e = _currentClassOrNamespace->lookupClassOrNamespace(namedTy->name())) {
                    _currentClassOrNamespace->addNestedClassOrNamespace(decl->name(), e);
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
    if (ClassOrNamespace *base = _currentClassOrNamespace->lookupClassOrNamespace(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(UsingNamespaceDirective *u)
{
    if (ClassOrNamespace *e = _currentClassOrNamespace->lookupClassOrNamespace(u->name())) {
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

    } else if (ClassOrNamespace *e = _currentClassOrNamespace->lookupClassOrNamespace(a->namespaceName())) {
        if (a->name()->isNameId() || a->name()->isTemplateNameId())
            _currentClassOrNamespace->addNestedClassOrNamespace(a->name(), e);

    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo(a->namespaceName());
    }

    return false;
}

bool CreateBindings::visit(ObjCClass *klass)
{
    ClassOrNamespace *previous = enterGlobalEntity(klass);

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
    if (ClassOrNamespace *base = _globalNamespace->lookupClassOrNamespace(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardClassDeclaration *klass)
{
    ClassOrNamespace *previous = enterGlobalEntity(klass);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCProtocol *proto)
{
    ClassOrNamespace *previous = enterGlobalEntity(proto);

    for (unsigned i = 0; i < proto->protocolCount(); ++i)
        process(proto->protocolAt(i));

    for (unsigned i = 0; i < proto->memberCount(); ++i)
        process(proto->memberAt(i));

    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCBaseProtocol *b)
{
    if (ClassOrNamespace *base = _globalNamespace->lookupClassOrNamespace(b->name())) {
        _currentClassOrNamespace->addUsing(base);
    } else if (false) {
        Overview oo;
        qDebug() << "no entity for:" << oo(b->name());
    }
    return false;
}

bool CreateBindings::visit(ObjCForwardProtocolDeclaration *proto)
{
    ClassOrNamespace *previous = enterGlobalEntity(proto);
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(ObjCMethod *)
{
    return false;
}
