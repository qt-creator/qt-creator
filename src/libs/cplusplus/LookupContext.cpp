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
#include "DeprecatedGenTemplateInstance.h"
#include "CppRewriter.h"

#include <cplusplus/CoreTypes.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/Literals.h>
#include <cplusplus/Names.h>
#include <cplusplus/Scope.h>
#include <cplusplus/Control.h>

#include <QStack>
#include <QHash>
#include <QVarLengthArray>
#include <QDebug>

using namespace CPlusPlus;

static const bool debug = ! qgetenv("QTC_LOOKUPCONTEXT_DEBUG").isEmpty();

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

static bool isNestedInstantiationEnclosingTemplate(
        ClassOrNamespace *nestedClassOrNamespaceInstantiation,
        ClassOrNamespace *enclosingTemplateClassInstantiation)
{
    QList<ClassOrNamespace *> processed;
    while (enclosingTemplateClassInstantiation
           && !processed.contains(enclosingTemplateClassInstantiation)) {
        processed.append(enclosingTemplateClassInstantiation);
        if (enclosingTemplateClassInstantiation == nestedClassOrNamespaceInstantiation)
            return false;
        enclosingTemplateClassInstantiation = enclosingTemplateClassInstantiation->parent();
    }

    return true;
}

namespace CPlusPlus {

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

}

namespace CPlusPlus {
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
                             QSharedPointer<CreateBindings> bindings)
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

const Name *LookupContext::minimalName(Symbol *symbol, ClassOrNamespace *target, Control *control)
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
                                               ClassOrNamespace *bindingScope) const
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
                ClassOrNamespace *base = lookupType(q->base(), scope);
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

ClassOrNamespace *LookupContext::globalNamespace() const
{
    return bindings()->globalNamespace();
}

ClassOrNamespace *LookupContext::lookupType(const Name *name, Scope *scope,
                                            ClassOrNamespace *enclosingBinding,
                                            QSet<const Declaration *> typedefsBeingResolved) const
{
    if (! scope || ! name) {
        return 0;
    } else if (Block *block = scope->asBlock()) {
        for (unsigned i = 0; i < block->memberCount(); ++i) {
            Symbol *m = block->memberAt(i);
            if (UsingNamespaceDirective *u = m->asUsingNamespaceDirective()) {
                if (ClassOrNamespace *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                    if (ClassOrNamespace *r = uu->lookupType(name))
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
        if (ClassOrNamespace *b = bindings()->lookupType(scope, enclosingBinding)) {
            if (ClassOrNamespace *classOrNamespaceNestedInNestedBlock = b->lookupType(name, block))
                return classOrNamespaceNestedInNestedBlock;
        }

        // try to find type in enclosing scope(typical case)
        if (ClassOrNamespace *found = lookupType(name, scope->enclosingScope()))
            return found;

    } else if (ClassOrNamespace *b = bindings()->lookupType(scope, enclosingBinding)) {
        return b->lookupType(name);
    } else if (Class *scopeAsClass = scope->asClass()) {
        if (scopeAsClass->enclosingScope()->isBlock()) {
            if (ClassOrNamespace *b = lookupType(scopeAsClass->name(),
                                                 scopeAsClass->enclosingScope(),
                                                 enclosingBinding,
                                                 typedefsBeingResolved)) {
                return b->lookupType(name);
            }
        }
    }

    return 0;
}

ClassOrNamespace *LookupContext::lookupType(Symbol *symbol,
                                            ClassOrNamespace *enclosingBinding) const
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
            bindings()->lookupInScope(name, scope, &candidates, /*templateId = */ 0, /*binding=*/ 0);

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
                    if (ClassOrNamespace *uu = lookupType(u->name(), scope->enclosingNamespace())) {
                        candidates = uu->find(name);

                        if (! candidates.isEmpty())
                            return candidates;
                    }
                }
            }

            if (ClassOrNamespace *bindingScope = bindings()->lookupType(scope)) {
                if (ClassOrNamespace *bindingBlock = bindingScope->findBlock(scope->asBlock())) {
                    candidates = lookupByUsing(name, bindingBlock);
                    if (! candidates.isEmpty())
                        return candidates;

                    candidates = bindingBlock->find(name);

                    if (! candidates.isEmpty())
                        return candidates;
                }
            }

        } else if (Function *fun = scope->asFunction()) {
            bindings()->lookupInScope(name, fun, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty()) {
                // it's an argument or a template parameter.
                //for qualified it can be outside of the local scope
                if (name->isQualifiedNameId())
                    continue;
                else
                    break;
            }

            if (fun->name() && fun->name()->isQualifiedNameId()) {
                if (ClassOrNamespace *binding = bindings()->lookupType(fun)) {
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
            bindings()->lookupInScope(name, method, &candidates, /*templateId = */ 0, /*binding=*/ 0);

            if (! candidates.isEmpty())
                break; // it's a formal argument.

        } else if (Template *templ = scope->asTemplate()) {
            bindings()->lookupInScope(name, templ, &candidates, /*templateId = */ 0, /*binding=*/ 0);

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

            if (ClassOrNamespace *bindingScope = bindings()->lookupType(scope)) {
                candidates = bindingScope->find(name);

                if (! candidates.isEmpty())
                    return candidates;

                candidates = lookupByUsing(name, bindingScope);
                if (!candidates.isEmpty())
                    return candidates;
            }

            // the scope can be defined inside a block, try to find it
            if (Block *block = scope->enclosingBlock()) {
                if (ClassOrNamespace *b = bindings()->lookupType(block)) {
                    if (ClassOrNamespace *classOrNamespaceNestedInNestedBlock = b->lookupType(scope->name(), block))
                        candidates = classOrNamespaceNestedInNestedBlock->find(name);
                }
            }

            if (! candidates.isEmpty())
                return candidates;

        } else if (scope->isObjCClass() || scope->isObjCProtocol()) {
            if (ClassOrNamespace *binding = bindings()->lookupType(scope))
                candidates = binding->find(name);

                if (! candidates.isEmpty())
                    return candidates;
        }
    }

    return candidates;
}

ClassOrNamespace *LookupContext::lookupParent(Symbol *symbol) const
{
    QList<const Name *> fqName = path(symbol);
    ClassOrNamespace *binding = globalNamespace();
    foreach (const Name *name, fqName) {
        binding = binding->findType(name);
        if (!binding)
            return 0;
    }

    return binding;
}

ClassOrNamespace::ClassOrNamespace(CreateBindings *factory, ClassOrNamespace *parent)
    : _factory(factory)
    , _parent(parent)
    , _scopeLookupCache(0)
    , _templateId(0)
    , _instantiationOrigin(0)
    , _rootClass(0)
    , _name(0)
{
    Q_ASSERT(factory);
}

ClassOrNamespace::~ClassOrNamespace()
{
    delete _scopeLookupCache;
}

const TemplateNameId *ClassOrNamespace::templateId() const
{
    return _templateId;
}

ClassOrNamespace *ClassOrNamespace::instantiationOrigin() const
{
    return _instantiationOrigin;
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

QList<Enum *> ClassOrNamespace::unscopedEnums() const
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

QList<LookupItem> ClassOrNamespace::find(const Name *name)
{
    return lookup_helper(name, false);
}

QList<LookupItem> ClassOrNamespace::lookup(const Name *name)
{
    return lookup_helper(name, true);
}

QList<LookupItem> ClassOrNamespace::lookup_helper(const Name *name, bool searchInEnclosingScope)
{
    QList<LookupItem> result;

    if (name) {

        if (const QualifiedNameId *q = name->asQualifiedNameId()) {
            if (! q->base()) { // e.g. ::std::string
                result = globalNamespace()->find(q->name());
            } else if (ClassOrNamespace *binding = lookupType(q->base())) {
                result = binding->find(q->name());

                QList<const Name *> fullName;
                addNames(name, &fullName);

                // It's also possible that there are matches in the parent binding through
                // a qualified name. For instance, a nested class which is forward declared
                // in the class but defined outside it - we should capture both.
                Symbol *match = 0;
                QSet<ClassOrNamespace *> processed;
                for (ClassOrNamespace *parentBinding = binding->parent();
                        parentBinding && !match;
                        parentBinding = parentBinding->parent()) {
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

        QSet<ClassOrNamespace *> processed;
        QSet<ClassOrNamespace *> processedOwnParents;
        ClassOrNamespace *binding = this;
        do {
            if (processedOwnParents.contains(binding))
                break;
            processedOwnParents.insert(binding);
            lookup_helper(name, binding, &result, &processed, /*templateId = */ 0);
            binding = binding->_parent;
        } while (searchInEnclosingScope && binding);
    }

    return result;
}

void ClassOrNamespace::lookup_helper(const Name *name, ClassOrNamespace *binding,
                                          QList<LookupItem> *result,
                                          QSet<ClassOrNamespace *> *processed,
                                          const TemplateNameId *templateId)
{
    if (binding && ! processed->contains(binding)) {
        processed->insert(binding);

        const Identifier *nameId = name->identifier();

        foreach (Symbol *s, binding->symbols()) {
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
                            item.setBinding(binding);
                            result->append(item);
                        }
                    }
                }
                _factory->lookupInScope(name, scope, result, templateId, binding);
            }
        }

        foreach (Enum *e, binding->unscopedEnums())
            _factory->lookupInScope(name, e, result, templateId, binding);

        foreach (ClassOrNamespace *u, binding->usings())
            lookup_helper(name, u, result, processed, binding->_templateId);

        Anonymouses::const_iterator cit = binding->_anonymouses.constBegin();
        Anonymouses::const_iterator citEnd = binding->_anonymouses.constEnd();
        for (; cit != citEnd; ++cit) {
            const AnonymousNameId *anonymousNameId = cit.key();
            ClassOrNamespace *a = cit.value();
            if (!binding->_declaredOrTypedefedAnonymouses.contains(anonymousNameId))
                lookup_helper(name, a, result, processed, binding->_templateId);
        }
    }
}

void CreateBindings::lookupInScope(const Name *name, Scope *scope,
                                   QList<LookupItem> *result,
                                   const TemplateNameId *templateId,
                                   ClassOrNamespace *binding)
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
                         << (binding ? oo(binding->_name) : QString::fromLatin1("<null>"));
            }

            LookupItem item;
            item.setDeclaration(s);
            item.setBinding(binding);

            if (s->asNamespaceAlias() && binding) {
                ClassOrNamespace *targetNamespaceBinding = binding->lookupType(name);
                //there can be many namespace definitions
                if (targetNamespaceBinding && targetNamespaceBinding->symbols().size() > 0) {
                    Symbol *resolvedSymbol = targetNamespaceBinding->symbols().first();
                    item.setType(resolvedSymbol->type()); // override the type
                }
            }

            if (templateId && (s->isDeclaration() || s->isFunction())) {
                FullySpecifiedType ty = DeprecatedGenTemplateInstance::instantiate(templateId, s, control());
                item.setType(ty); // override the type.
            }

            // instantiate function template
            if (name->isTemplateNameId() && s->isTemplate() && s->asTemplate()->declaration()
                    && s->asTemplate()->declaration()->isFunction()) {
                const TemplateNameId *instantiation = name->asTemplateNameId();
                Template *specialization = s->asTemplate();
                Symbol *instantiatedFunctionTemplate = instantiateTemplateFunction(instantiation,
                                                                                   specialization);
                item.setType(instantiatedFunctionTemplate->type()); // override the type.
            }

            result->append(item);
        }
    }
}

ClassOrNamespace *ClassOrNamespace::lookupType(const Name *name)
{
    if (! name)
        return 0;

    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ true, this);
}

ClassOrNamespace *ClassOrNamespace::lookupType(const Name *name, Block *block)
{
    flush();

    QHash<Block *, ClassOrNamespace *>::const_iterator citBlock = _blocks.constFind(block);
    if (citBlock != _blocks.constEnd()) {
        ClassOrNamespace *nestedBlock = citBlock.value();
        QSet<ClassOrNamespace *> processed;
        if (ClassOrNamespace *foundInNestedBlock
                = nestedBlock->lookupType_helper(name,
                                                 &processed,
                                                 /*searchInEnclosingScope = */ true,
                                                 this)) {
            return foundInNestedBlock;
        }
    }

    for (citBlock = _blocks.constBegin(); citBlock != _blocks.constEnd(); ++citBlock) {
        if (ClassOrNamespace *foundNestedBlock = citBlock.value()->lookupType(name, block))
            return foundNestedBlock;
    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::findType(const Name *name)
{
    QSet<ClassOrNamespace *> processed;
    return lookupType_helper(name, &processed, /*searchInEnclosingScope =*/ false, this);
}

ClassOrNamespace *ClassOrNamespace::findBlock_helper(Block *block,
                                                     QSet<ClassOrNamespace *> *processed,
                                                     bool searchInEnclosingScope)
{
    for (ClassOrNamespace *binding = this; binding; binding = binding->_parent) {
        if (processed->contains(binding))
            break;
        processed->insert(binding);
        binding->flush();
        auto end = binding->_blocks.end();
        auto citBlock = binding->_blocks.find(block);
        if (citBlock != end)
            return citBlock.value();

        for (citBlock = binding->_blocks.begin(); citBlock != end; ++citBlock) {
            if (ClassOrNamespace *foundNestedBlock =
                    citBlock.value()->findBlock_helper(block, processed, false)) {
                return foundNestedBlock;
            }
        }
        if (!searchInEnclosingScope)
            break;
    }
    return 0;
}

ClassOrNamespace *ClassOrNamespace::findBlock(Block *block)
{
    QSet<ClassOrNamespace *> processed;
    return findBlock_helper(block, &processed, true);
}

Symbol *ClassOrNamespace::lookupInScope(const QList<const Name *> &fullName)
{
    if (!_scopeLookupCache) {
        _scopeLookupCache = new QHash<Internal::FullyQualifiedName, Symbol *>;

        for (int j = 0; j < symbols().size(); ++j) {
            if (Scope *scope = symbols().at(j)->asScope()) {
                for (unsigned i = 0; i < scope->memberCount(); ++i) {
                    Symbol *s = scope->memberAt(i);
                    _scopeLookupCache->insert(LookupContext::fullyQualifiedName(s), s);
                }
            }
        }
    }

    return _scopeLookupCache->value(fullName, 0);
}

ClassOrNamespace *ClassOrNamespace::lookupType_helper(const Name *name,
                                                      QSet<ClassOrNamespace *> *processed,
                                                      bool searchInEnclosingScope,
                                                      ClassOrNamespace *origin)
{
    if (Q_UNLIKELY(debug)) {
        Overview oo;
        qDebug() << "Looking up" << oo(name) << "in" << oo(_name);
    }

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {

        QSet<ClassOrNamespace *> innerProcessed;
        if (! q->base())
            return globalNamespace()->lookupType_helper(q->name(), &innerProcessed, true, origin);

        if (ClassOrNamespace *binding = lookupType_helper(q->base(), processed, true, origin))
            return binding->lookupType_helper(q->name(), &innerProcessed, false, origin);

        return 0;

    } else if (! processed->contains(this)) {
        processed->insert(this);

        if (name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId()) {
            flush();

            foreach (Symbol *s, symbols()) {
                if (Class *klass = s->asClass()) {
                    if (klass->identifier() && klass->identifier()->match(name->identifier()))
                        return this;
                }
            }
            foreach (Enum *e, unscopedEnums()) {
                if (e->identifier() && e->identifier()->match(name->identifier()))
                    return this;
            }

            if (ClassOrNamespace *e = nestedType(name, origin))
                return e;

            if (_templateId) {
                if (_usings.size() == 1) {
                    ClassOrNamespace *delegate = _usings.first();

                    if (ClassOrNamespace *r = delegate->lookupType_helper(name,
                                                                          processed,
                                                                          /*searchInEnclosingScope = */ true,
                                                                          origin))
                        return r;
                } else if (Q_UNLIKELY(debug)) {
                    qWarning() << "expected one using declaration. Number of using declarations is:"
                               << _usings.size();
                }
            }

            foreach (ClassOrNamespace *u, usings()) {
                if (ClassOrNamespace *r = u->lookupType_helper(name,
                                                               processed,
                                                               /*searchInEnclosingScope =*/ false,
                                                               origin))
                    return r;
            }
        }

        if (_parent && searchInEnclosingScope)
            return _parent->lookupType_helper(name, processed, searchInEnclosingScope, origin);
    }

    return 0;
}

static ClassOrNamespace *findSpecializationWithMatchingTemplateArgument(const Name *argumentName,
                                                                        ClassOrNamespace *reference)
{
    foreach (Symbol *s, reference->symbols()) {
        if (Class *clazz = s->asClass()) {
            if (Template *templateSpecialization = clazz->enclosingTemplate()) {
                const unsigned argumentCountOfSpecialization
                                    = templateSpecialization->templateParameterCount();
                for (unsigned i = 0; i < argumentCountOfSpecialization; ++i) {
                    if (TypenameArgument *tParam
                            = templateSpecialization->templateParameterAt(i)->asTypenameArgument()) {
                        if (const Name *name = tParam->name()) {
                            if (compareName(name, argumentName))
                                return reference;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

ClassOrNamespace *ClassOrNamespace::findSpecialization(const TemplateNameId *templId,
                                                       const TemplateNameIdTable &specializations)
{
    // we go through all specialization and try to find that one with template argument as pointer
    for (TemplateNameIdTable::const_iterator cit = specializations.begin();
         cit != specializations.end(); ++cit) {
        const TemplateNameId *specializationNameId = cit->first;
        const unsigned specializationTemplateArgumentCount
                = specializationNameId->templateArgumentCount();
        const unsigned initializationTemplateArgumentCount
                = templId->templateArgumentCount();
        // for now it works only when we have the same number of arguments in specialization
        // and initialization(in future it should be more clever)
        if (specializationTemplateArgumentCount == initializationTemplateArgumentCount) {
            for (unsigned i = 0; i < initializationTemplateArgumentCount; ++i) {
                const FullySpecifiedType &specializationTemplateArgument
                        = specializationNameId->templateArgumentAt(i);
                const FullySpecifiedType &initializationTemplateArgument
                        = templId->templateArgumentAt(i);
                PointerType *specPointer
                        = specializationTemplateArgument.type()->asPointerType();
                // specialization and initialization argument have to be a pointer
                // additionally type of pointer argument of specialization has to be namedType
                if (specPointer && initializationTemplateArgument.type()->isPointerType()
                        && specPointer->elementType().type()->isNamedType()) {
                    return cit->second;
                }

                ArrayType *specArray
                        = specializationTemplateArgument.type()->asArrayType();
                if (specArray && initializationTemplateArgument.type()->isArrayType()) {
                    if (const NamedType *argumentNamedType
                            = specArray->elementType().type()->asNamedType()) {
                        if (const Name *argumentName = argumentNamedType->name()) {
                            if (ClassOrNamespace *reference
                                    = findSpecializationWithMatchingTemplateArgument(
                                            argumentName, cit->second)) {
                                return reference;
                            }
                        }
                    }
                }
            }
        }
    }

    return 0;
}

ClassOrNamespace *ClassOrNamespace::findOrCreateNestedAnonymousType(
        const AnonymousNameId *anonymousNameId)
{
    QHash<const AnonymousNameId *, ClassOrNamespace *>::const_iterator cit
            = _anonymouses.constFind(anonymousNameId);
    if (cit != _anonymouses.constEnd()) {
        return cit.value();
    } else {
        ClassOrNamespace *newAnonymous = _factory->allocClassOrNamespace(this);
        if (Q_UNLIKELY(debug))
            newAnonymous->_name = anonymousNameId;
        _anonymouses[anonymousNameId] = newAnonymous;
        return newAnonymous;
    }
}

ClassOrNamespace *ClassOrNamespace::nestedType(const Name *name, ClassOrNamespace *origin)
{
    Q_ASSERT(name != 0);
    Q_ASSERT(name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId());

    const_cast<ClassOrNamespace *>(this)->flush();

    if (const AnonymousNameId *anonymousNameId = name->asAnonymousNameId())
        return findOrCreateNestedAnonymousType(anonymousNameId);

    Table::const_iterator it = _classOrNamespaces.find(name);
    if (it == _classOrNamespaces.end())
        return 0;

    ClassOrNamespace *reference = it->second;
    ClassOrNamespace *baseTemplateClassReference = reference;

    const TemplateNameId *templId = name->asTemplateNameId();
    if (templId) {
        // for "using" we should use the real one ClassOrNamespace(it should be the first
        // one item from usings list)
        // we indicate that it is a 'using' by checking number of symbols(it should be 0)
        if (reference->symbols().count() == 0 && reference->usings().count() != 0)
            reference = reference->_usings[0];

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
                ClassOrNamespace *newSpecialization = _factory->allocClassOrNamespace(reference);
                if (Q_UNLIKELY(debug))
                    newSpecialization->_name = templId;
                reference->_specializations[templId] = newSpecialization;
                return newSpecialization;
            }
        } else {
            QMap<const TemplateNameId *, ClassOrNamespace *>::const_iterator citInstantiation
                    = reference->_instantiations.constFind(templId);
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
                ClassOrNamespace *specializationWithPointer
                        = findSpecialization(templId, specializations);
                if (specializationWithPointer)
                    reference = specializationWithPointer;
                // TODO: find the best specialization(probably partial) for this instantiation
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
    foreach (Symbol *s, reference->symbols()) {
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

    QSet<ClassOrNamespace *> knownUsings = reference->usings().toSet();

    // If we are dealling with a template type, more work is required, since we need to
    // construct all instantiation data.
    if (templId) {
        _alreadyConsideredTemplates.insert(templId);
        ClassOrNamespace *instantiation = _factory->allocClassOrNamespace(baseTemplateClassReference);
        if (Q_UNLIKELY(debug))
            instantiation->_name = templId;
        instantiation->_templateId = templId;

        while (!origin->_symbols.isEmpty() && origin->_symbols[0]->isBlock())
            origin = origin->parent();

        instantiation->_instantiationOrigin = origin;

        // The instantiation should have all symbols, enums, and usings from the reference.
        instantiation->_enums.append(reference->unscopedEnums());
        instantiation->_usings.append(reference->usings());

        instantiation->_rootClass = reference->rootClass();

        // It gets a bit complicated if the reference is actually a class template because we
        // now must worry about dependent names in base classes.
        if (Template *templateSpecialization = referenceClass->enclosingTemplate()) {
            const unsigned argumentCountOfInitialization = templId->templateArgumentCount();
            const unsigned argumentCountOfSpecialization
                    = templateSpecialization->templateParameterCount();

            Subst subst(_control.data());
            if (_factory->expandTemplates()) {
                const TemplateNameId *templSpecId
                        = templateSpecialization->name()->asTemplateNameId();
                const unsigned templSpecArgumentCount = templSpecId ?
                            templSpecId->templateArgumentCount() : 0;
                Clone cloner(_control.data());
                for (unsigned i = 0; i < argumentCountOfSpecialization; ++i) {
                    const TypenameArgument *tParam
                            = templateSpecialization->templateParameterAt(i)->asTypenameArgument();
                    if (!tParam)
                        continue;
                    const Name *name = tParam->name();
                    if (!name)
                        continue;

                    FullySpecifiedType ty = (i < argumentCountOfInitialization) ?
                                templId->templateArgumentAt(i):
                                cloner.type(tParam->type(), &subst);

                    if (i < templSpecArgumentCount
                            && templSpecId->templateArgumentAt(i)->isPointerType()) {
                        if (PointerType *pointerType = ty->asPointerType())
                            ty = pointerType->elementType();
                    }

                    subst.bind(cloner.name(name, &subst), ty);
                }

                foreach (Symbol *s, reference->symbols()) {
                    Symbol *clone = cloner.symbol(s, &subst);
                    clone->setEnclosingScope(s->enclosingScope());
                    instantiation->_symbols.append(clone);
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
                instantiateNestedClasses(reference, cloner, subst, instantiation);
            } else {
                instantiation->_symbols.append(reference->symbols());
            }

            QHash<const Name*, unsigned> templParams;
            for (unsigned i = 0; i < argumentCountOfSpecialization; ++i)
                templParams.insert(templateSpecialization->templateParameterAt(i)->name(), i);

            foreach (const Name *baseName, allBases) {
                ClassOrNamespace *baseBinding = 0;

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
                                    baseBinding = lookupType(namedType->name());
                            }
                        }
                    }
                    if (!baseBinding && subst.contains(baseName)) {
                        const FullySpecifiedType &fullType = subst[baseName];
                        if (fullType.isValid()) {
                            if (NamedType *namedType = fullType.type()->asNamedType())
                                baseBinding = lookupType(namedType->name());
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
                        if (baseTemplId->identifier() != templId->identifier())
                            baseBinding = nestedType(baseName, origin);
                    } else if (const QualifiedNameId *qBaseName = baseName->asQualifiedNameId()) {
                        // Qualified names in general.
                        // Ex.: template <class T> class A : public B<T>::Type {};
                        ClassOrNamespace *binding = this;
                        if (const Name *qualification = qBaseName->base()) {
                            const TemplateNameId *baseTemplName = qualification->asTemplateNameId();
                            if (!baseTemplName || !compareName(baseTemplName, templateSpecialization->name()))
                                binding = lookupType(qualification);
                        }
                        baseName = qBaseName->name();

                        if (binding)
                            baseBinding = binding->lookupType(baseName);
                    }
                }

                if (baseBinding && !knownUsings.contains(baseBinding))
                    instantiation->addUsing(baseBinding);
            }
        } else {
            instantiation->_classOrNamespaces = reference->_classOrNamespaces;
            instantiation->_symbols.append(reference->symbols());
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
        ClassOrNamespace *binding = this;
        if (const QualifiedNameId *qBaseName = baseName->asQualifiedNameId()) {
            if (const Name *qualification = qBaseName->base())
                binding = lookupType(qualification);
            else if (binding->parent() != 0)
                //if this is global identifier we take global namespace
                //Ex: class A{}; namespace NS { class A: public ::A{}; }
                binding = binding->globalNamespace();
            else
                //if we are in the global scope
                continue;
            baseName = qBaseName->name();
        }

        if (binding) {
            ClassOrNamespace * baseBinding = binding->lookupType(baseName);
            if (baseBinding && !knownUsings.contains(baseBinding))
                reference->addUsing(baseBinding);
        }
    }

    _alreadyConsideredClasses.clear(referenceClass);
    return reference;
}


void ClassOrNamespace::instantiateNestedClasses(ClassOrNamespace *enclosingTemplateClass,
                                                Clone &cloner,
                                                Subst &subst,
                                                ClassOrNamespace *enclosingTemplateClassInstantiation)
{
    NestedClassInstantiator nestedClassInstantiator(_factory, cloner, subst);
    nestedClassInstantiator.instantiate(enclosingTemplateClass, enclosingTemplateClassInstantiation);
}

void ClassOrNamespace::NestedClassInstantiator::instantiate(ClassOrNamespace *enclosingTemplateClass,
                                                ClassOrNamespace *enclosingTemplateClassInstantiation)
{
    if (_alreadyConsideredNestedClassInstantiations.contains(enclosingTemplateClass))
        return;
    _alreadyConsideredNestedClassInstantiations.insert(enclosingTemplateClass);
    ClassOrNamespace::Table::const_iterator cit = enclosingTemplateClass->_classOrNamespaces.begin();
    for (; cit != enclosingTemplateClass->_classOrNamespaces.end(); ++cit) {
        const Name *nestedName = cit->first;
        ClassOrNamespace *nestedClassOrNamespace = cit->second;
        ClassOrNamespace *nestedClassOrNamespaceInstantiation = nestedClassOrNamespace;

        if (isInstantiateNestedClassNeeded(nestedClassOrNamespace->_symbols)) {
            nestedClassOrNamespaceInstantiation = _factory->allocClassOrNamespace(nestedClassOrNamespace);
            nestedClassOrNamespaceInstantiation->_enums.append(nestedClassOrNamespace->unscopedEnums());
            nestedClassOrNamespaceInstantiation->_usings.append(nestedClassOrNamespace->usings());
            nestedClassOrNamespaceInstantiation->_instantiationOrigin = nestedClassOrNamespace;

            foreach (Symbol *s, nestedClassOrNamespace->_symbols) {
                Symbol *clone = _cloner.symbol(s, &_subst);
                if (!clone->enclosingScope()) // Not from the cache but just cloned.
                    clone->setEnclosingScope(s->enclosingScope());
                nestedClassOrNamespaceInstantiation->_symbols.append(clone);
            }
        }

        if (isNestedInstantiationEnclosingTemplate(nestedClassOrNamespaceInstantiation,
                                                   enclosingTemplateClass)) {
            nestedClassOrNamespaceInstantiation->_parent = enclosingTemplateClassInstantiation;
        }
        instantiate(nestedClassOrNamespace, nestedClassOrNamespaceInstantiation);

        enclosingTemplateClassInstantiation->_classOrNamespaces[nestedName] =
                nestedClassOrNamespaceInstantiation;
    }
    _alreadyConsideredNestedClassInstantiations.remove(enclosingTemplateClass);
}

bool ClassOrNamespace::NestedClassInstantiator::isInstantiateNestedClassNeeded(const QList<Symbol *> &symbols) const
{
    foreach (Symbol *s, symbols) {
        if (Class *klass = s->asClass()) {
            int memberCount = klass->memberCount();
            for (int i = 0; i < memberCount; ++i) {
                Symbol *memberAsSymbol = klass->memberAt(i);
                if (Declaration *declaration = memberAsSymbol->asDeclaration()) {
                    if (containsTemplateType(declaration))
                        return true;
                } else if (Function *function = memberAsSymbol->asFunction()) {
                    if (containsTemplateType(function))
                        return true;
                }
            }
        }
    }

    return false;
}

bool ClassOrNamespace::NestedClassInstantiator::containsTemplateType(Declaration *declaration) const
{
    Type *memberType = declaration->type().type();
    NamedType *namedType = findNamedType(memberType);
    return namedType && _subst.contains(namedType->name());
}

bool ClassOrNamespace::NestedClassInstantiator::containsTemplateType(Function *function) const
{
    Type *returnType = function->returnType().type();
    NamedType *namedType = findNamedType(returnType);
    return namedType && _subst.contains(namedType->name());
    //TODO: in future we will need also check function arguments, for now returned value is enough
}

NamedType *ClassOrNamespace::NestedClassInstantiator::findNamedType(Type *memberType) const
{
    if (NamedType *namedType = memberType->asNamedType())
        return namedType;
    else if (PointerType *pointerType = memberType->asPointerType())
        return findNamedType(pointerType->elementType().type());
    else if (ReferenceType *referenceType = memberType->asReferenceType())
        return findNamedType(referenceType->elementType().type());

    return 0;
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

void ClassOrNamespace::addUnscopedEnum(Enum *e)
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

ClassOrNamespace *ClassOrNamespace::findOrCreateType(const Name *name, ClassOrNamespace *origin,
                                                     Class *clazz)
{
    if (! name)
        return this;
    if (! origin)
        origin = this;

    if (const QualifiedNameId *q = name->asQualifiedNameId()) {
        if (! q->base())
            return globalNamespace()->findOrCreateType(q->name(), origin, clazz);

        return findOrCreateType(q->base(), origin)->findOrCreateType(q->name(), origin, clazz);

    } else if (name->isNameId() || name->isTemplateNameId() || name->isAnonymousNameId()) {
        ClassOrNamespace *e = nestedType(name, origin);

        if (! e) {
            e = _factory->allocClassOrNamespace(this);
            e->_rootClass = clazz;
            if (Q_UNLIKELY(debug))
                e->_name = name;
            _classOrNamespaces[name] = e;
        }

        return e;
    }

    return 0;
}

CreateBindings::CreateBindings(Document::Ptr thisDocument, const Snapshot &snapshot)
    : _snapshot(snapshot)
    , _control(QSharedPointer<Control>(new Control))
    , _expandTemplates(false)
{
    _globalNamespace = allocClassOrNamespace(/*parent = */ 0);
    _currentClassOrNamespace = _globalNamespace;

    process(thisDocument);
}

CreateBindings::~CreateBindings()
{
    qDeleteAll(_entities);
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

ClassOrNamespace *CreateBindings::lookupType(Symbol *symbol, ClassOrNamespace *enclosingBinding)
{
    const QList<const Name *> path = LookupContext::path(symbol);
    return lookupType(path, enclosingBinding);
}

ClassOrNamespace *CreateBindings::lookupType(const QList<const Name *> &path,
                                             ClassOrNamespace *enclosingBinding)
{
    if (path.isEmpty())
        return _globalNamespace;

    if (enclosingBinding) {
        if (ClassOrNamespace *b = enclosingBinding->lookupType(path.last()))
            return b;
    }

    ClassOrNamespace *b = _globalNamespace->lookupType(path.at(0));

    for (int i = 1; b && i < path.size(); ++i)
        b = b->findType(path.at(i));

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

ClassOrNamespace *CreateBindings::allocClassOrNamespace(ClassOrNamespace *parent)
{
    ClassOrNamespace *e = new ClassOrNamespace(this, parent);
    e->_control = control();
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

ClassOrNamespace *CreateBindings::enterClassOrNamespaceBinding(Symbol *symbol)
{
    ClassOrNamespace *entity = _currentClassOrNamespace->findOrCreateType(symbol->name(), 0,
                                                                          symbol->asClass());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

ClassOrNamespace *CreateBindings::enterGlobalClassOrNamespace(Symbol *symbol)
{
    ClassOrNamespace *entity = _globalNamespace->findOrCreateType(symbol->name(), 0,
                                                                  symbol->asClass());
    entity->addSymbol(symbol);

    return switchCurrentClassOrNamespace(entity);
}

bool CreateBindings::visit(Template *templ)
{
    if (Symbol *d = templ->declaration())
        accept(d);

    return false;
}

bool CreateBindings::visit(Namespace *ns)
{
    ClassOrNamespace *previous = enterClassOrNamespaceBinding(ns);

    for (unsigned i = 0; i < ns->memberCount(); ++i)
        process(ns->memberAt(i));

    if (ns->isInline() && previous)
        previous->addUsing(_currentClassOrNamespace);

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
        binding = _currentClassOrNamespace->findOrCreateType(klass->name(), 0, klass);

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
    if (e->isScoped()) {
        ClassOrNamespace *previous = enterClassOrNamespaceBinding(e);
        _currentClassOrNamespace = previous;
    } else {
        _currentClassOrNamespace->addUnscopedEnum(e);
    }
    return false;
}

bool CreateBindings::visit(Declaration *decl)
{
    if (decl->isTypedef()) {
        FullySpecifiedType ty = decl->type();
        const Identifier *typedefId = decl->identifier();

        if (typedefId && ! (ty.isConst() || ty.isVolatile())) {
            if (const NamedType *namedTy = ty->asNamedType()) {
                if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(namedTy->name())) {
                    _currentClassOrNamespace->addNestedType(decl->name(), e);
                } else if (false) {
                    Overview oo;
                    qDebug() << "found entity not found for" << oo.prettyName(namedTy->name());
                }
            } else if (Class *klass = ty->asClassType()) {
                if (const Identifier *nameId = decl->name()->asNameId()) {
                    ClassOrNamespace *binding
                        = _currentClassOrNamespace->findOrCreateType(nameId, 0, klass);
                    binding->addSymbol(klass);
                }
            }
        }
    }
    if (Class *clazz = decl->type()->asClassType()) {
        if (const Name *name = clazz->name()) {
            if (const AnonymousNameId *anonymousNameId = name->asAnonymousNameId())
                _currentClassOrNamespace->_declaredOrTypedefedAnonymouses.insert(anonymousNameId);
        }
    }
    return false;
}

bool CreateBindings::visit(Function *function)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;
    ClassOrNamespace *binding = lookupType(function, previous);
    if (!binding)
        return false;
    _currentClassOrNamespace = binding;
    for (unsigned i = 0, count = function->memberCount(); i < count; ++i) {
        Symbol *s = function->memberAt(i);
        if (Block *b = s->asBlock())
            visit(b);
    }
    _currentClassOrNamespace = previous;
    return false;
}

bool CreateBindings::visit(Block *block)
{
    ClassOrNamespace *previous = _currentClassOrNamespace;

    ClassOrNamespace *binding = new ClassOrNamespace(this, previous);
    binding->_control = control();

    _currentClassOrNamespace = binding;
    _currentClassOrNamespace->addSymbol(block);

    for (unsigned i = 0; i < block->memberCount(); ++i)
        // we cannot use lazy processing here, because we have to know
        // does this block contain any other blocks or classOrNamespaces
        process(block->memberAt(i), _currentClassOrNamespace);

    // we add this block to parent ClassOrNamespace only if it contains
    // any nested ClassOrNamespaces or other blocks(which have to contain
    // nested ClassOrNamespaces)
    if (! _currentClassOrNamespace->_blocks.empty()
            || ! _currentClassOrNamespace->_classOrNamespaces.empty()
            || ! _currentClassOrNamespace->_enums.empty()
            || ! _currentClassOrNamespace->_anonymouses.empty()) {
        previous->_blocks[block] = binding;
        _entities.append(binding);
    } else {
        delete binding;
        binding = 0;
    }

    _currentClassOrNamespace = previous;

    return false;
}

bool CreateBindings::visit(BaseClass *b)
{
    if (ClassOrNamespace *base = _currentClassOrNamespace->lookupType(b->name())) {
        _currentClassOrNamespace->addUsing(base);
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
                if (ClassOrNamespace *delegate = _currentClassOrNamespace->lookupType(q)) {
                    ClassOrNamespace *b = _currentClassOrNamespace->findOrCreateType(unqualifiedId);
                    b->addUsing(delegate);
                }
            }
        }
    }
    return false;
}

bool CreateBindings::visit(UsingNamespaceDirective *u)
{
    if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(u->name())) {
        _currentClassOrNamespace->addUsing(e);
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

    } else if (ClassOrNamespace *e = _currentClassOrNamespace->lookupType(a->namespaceName())) {
        if (a->name()->isNameId() || a->name()->isTemplateNameId() || a->name()->isAnonymousNameId())
            _currentClassOrNamespace->addNestedType(a->name(), e);

    } else if (false) {
        Overview oo;
        qDebug() << "no entity for namespace:" << oo.prettyName(a->namespaceName());
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
        qDebug() << "no entity for:" << oo.prettyName(b->name());
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
        qDebug() << "no entity for:" << oo.prettyName(b->name());
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

Symbol *CreateBindings::instantiateTemplateFunction(const TemplateNameId *instantiation,
                                                    Template *specialization) const
{
    const unsigned argumentCountOfInitialization = instantiation->templateArgumentCount();
    const unsigned argumentCountOfSpecialization = specialization->templateParameterCount();

    Clone cloner(_control.data());
    Subst subst(_control.data());
    for (unsigned i = 0; i < argumentCountOfSpecialization; ++i) {
        const TypenameArgument *tParam
                = specialization->templateParameterAt(i)->asTypenameArgument();
        if (!tParam)
            continue;
        const Name *name = tParam->name();
        if (!name)
            continue;

        FullySpecifiedType ty = (i < argumentCountOfInitialization) ?
                    instantiation->templateArgumentAt(i):
                    cloner.type(tParam->type(), &subst);

        subst.bind(cloner.name(name, &subst), ty);
    }
    return cloner.symbol(specialization, &subst);
}

