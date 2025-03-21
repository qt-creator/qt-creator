// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Control.h"
#include "CoreTypes.h"
#include "Literals.h"
#include "Matcher.h"
#include "Names.h"
#include "Scope.h"
#include "Symbols.h"
#include "SymbolVisitor.h"
#include "Templates.h"
#include "TypeVisitor.h"

#include <cstring>

using namespace CPlusPlus;

UsingNamespaceDirective::UsingNamespaceDirective(TranslationUnit *translationUnit,
                                                 int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingNamespaceDirective::UsingNamespaceDirective(Clone *clone, Subst *subst, UsingNamespaceDirective *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType UsingNamespaceDirective::type() const
{ return FullySpecifiedType(); }

void UsingNamespaceDirective::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


NamespaceAlias::NamespaceAlias(TranslationUnit *translationUnit,
                               int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name), _namespaceName(nullptr)
{ }

NamespaceAlias::NamespaceAlias(Clone *clone, Subst *subst, NamespaceAlias *original)
    : Symbol(clone, subst, original)
    , _namespaceName(clone->name(original->_namespaceName, subst))
{ }

FullySpecifiedType NamespaceAlias::type() const
{ return FullySpecifiedType(); }

void NamespaceAlias::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


UsingDeclaration::UsingDeclaration(TranslationUnit *translationUnit,
                                   int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

UsingDeclaration::UsingDeclaration(Clone *clone, Subst *subst, UsingDeclaration *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType UsingDeclaration::type() const
{ return FullySpecifiedType(); }

void CPlusPlus::UsingDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


Declaration::Declaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
    , _initializer(nullptr)
{ }

Declaration::Declaration(Clone *clone, Subst *subst, Declaration *original)
    : Symbol(clone, subst, original)
    , _type(clone->type(original->_type, subst))
    , _initializer(clone->stringLiteral(original->_initializer))
{
    const char* nameId = nullptr;
    const Name *theName = name();
    if (!theName)
        return;

    if (const Identifier* identifier = theName->identifier())
        nameId = identifier->chars();
    else
        return;

    Class *enClass = original->enclosingClass();
    const char* enClassNameId = nullptr;
    if (enClass && enClass->name() && enClass->name()->identifier()) {
        enClassNameId = enClass->name()->identifier()->chars();
    } else {
        return;
    }

    if (!enClassNameId)
        return;

    Template *templSpec  = enClass->enclosingTemplate();
    const char* enNamespaceNameId = nullptr;
    if (templSpec) {
        if (Namespace* ns = templSpec->enclosingNamespace()) {
            if (ns->isInline())
                ns = ns->enclosingNamespace();

            if (ns->name() && ns->name()->identifier())
                enNamespaceNameId =ns->name()->identifier()->chars();
        }
    }

    if (!enNamespaceNameId || templSpec->templateParameterCount() < 1)
        return;

    const Name *firstTemplParamName = nullptr;
    if (Symbol *param = templSpec->templateParameterAt(0);
        param->asTypenameArgument() || param->asTemplateTypeArgument())
        firstTemplParamName = param->name();

    if (!firstTemplParamName)
        return;

    if (!subst)
        return;

    FullySpecifiedType newType;
    if (std::strcmp(enNamespaceNameId, "std") == 0 ||
        std::strcmp(enNamespaceNameId, "__cxx11") == 0) {
        if (std::strcmp(enClassNameId, "unique_ptr") == 0) {
            if (std::strcmp(nameId, "pointer") == 0) {
                newType = clone->type(subst->apply(firstTemplParamName), nullptr);
                newType = FullySpecifiedType(clone->control()->pointerType(newType));
            }
        } else if (std::strcmp(enClassNameId, "list") == 0 ||
                   std::strcmp(enClassNameId, "forward_list") == 0 ||
                   std::strcmp(enClassNameId, "vector") == 0 ||
                   std::strcmp(enClassNameId, "queue") == 0 ||
                   std::strcmp(enClassNameId, "deque") == 0 ||
                   std::strcmp(enClassNameId, "set") == 0 ||
                   std::strcmp(enClassNameId, "unordered_set") == 0 ||
                   std::strcmp(enClassNameId, "multiset") == 0 ||
                   std::strcmp(enClassNameId, "array") == 0) {
            if (std::strcmp(nameId, "reference") == 0 ||
                std::strcmp(nameId, "const_reference") == 0) {
                newType = clone->type(subst->apply(firstTemplParamName), nullptr);
            } else if (std::strcmp(nameId, "iterator") == 0 ||
                       std::strcmp(nameId, "reverse_iterator") == 0 ||
                       std::strcmp(nameId, "const_reverse_iterator") == 0 ||
                       std::strcmp(nameId, "const_iterator") == 0) {
                newType = clone->type(subst->apply(firstTemplParamName), nullptr);
                newType = FullySpecifiedType(clone->control()->pointerType(newType));
            }
        } else if (std::strcmp(enClassNameId, "_Hash") == 0 ||
                   std::strcmp(enClassNameId, "_Tree") == 0 ) {
            if (std::strcmp(nameId, "iterator") == 0 ||
                std::strcmp(nameId, "reverse_iterator") == 0 ||
                std::strcmp(nameId, "const_reverse_iterator") == 0 ||
                std::strcmp(nameId, "const_iterator") == 0) {
                FullySpecifiedType clonedType = clone->type(subst->apply(firstTemplParamName), nullptr);
                if (NamedType *namedType = clonedType.type()->asNamedType()) {
                    if (const TemplateNameId * templateNameId =
                            namedType->name()->asTemplateNameId()) {
                        if (templateNameId->templateArgumentCount()) {
                            newType = clone->type(templateNameId->templateArgumentAt(0).type(), nullptr);
                            newType = FullySpecifiedType(clone->control()->pointerType(newType));
                        }
                    }
                }
            }
        }
    }

    if (newType.isValid())
        _type = newType;
}

void Declaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


EnumeratorDeclaration::EnumeratorDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Declaration(translationUnit, sourceLocation, name)
    , _constantValue(nullptr)
{}


Argument::Argument(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _initializer(nullptr)
{ }

Argument::Argument(Clone *clone, Subst *subst, Argument *original)
    : Symbol(clone, subst, original)
    , _initializer(clone->stringLiteral(original->_initializer))
    , _type(clone->type(original->_type, subst))
{ }

void Argument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


TypenameArgument::TypenameArgument(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
    , _isClassDeclarator(false)
{ }

TypenameArgument::TypenameArgument(Clone *clone, Subst *subst, TypenameArgument *original)
    : Symbol(clone, subst, original)
    , _type(clone->type(original->_type, subst))
    , _isClassDeclarator(original->_isClassDeclarator)
{ }

void TypenameArgument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


TemplateTypeArgument::TemplateTypeArgument(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

TemplateTypeArgument::TemplateTypeArgument(Clone *clone, Subst *subst, TemplateTypeArgument *original)
    : Symbol(clone, subst, original)
{ }

void TemplateTypeArgument::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

Function::Function(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name),
      _flags(0)
{ }

Function::Function(Clone *clone, Subst *subst, Function *original)
    : Scope(clone, subst, original)
    , _returnType(clone->type(original->_returnType, subst))
    , _exceptionSpecification(original->_exceptionSpecification)
    , _flags(original->_flags)
{ }

bool Function::isSignatureEqualTo(const Function *other, Matcher *matcher) const
{
    if (! other)
        return false;
    else if (isConst() != other->isConst())
        return false;
    else if (isVolatile() != other->isVolatile())
        return false;
    else if (! Matcher::match(unqualifiedName(), other->unqualifiedName(), matcher))
        return false;

    class FallbackMatcher : public Matcher
    {
    public:
        explicit FallbackMatcher(Matcher *baseMatcher) : m_baseMatcher(baseMatcher) {}

    private:
        bool match(const NamedType *type, const NamedType *otherType) override
        {
            if (type == otherType)
                 return true;
            const Name *name = type->name();
            if (const QualifiedNameId *q = name->asQualifiedNameId())
                name = q->name();
            const Name *otherName = otherType->name();
            if (const QualifiedNameId *q = otherName->asQualifiedNameId())
                otherName = q->name();
            return Matcher::match(name, otherName, m_baseMatcher);
        }

        Matcher * const m_baseMatcher;
    } fallbackMatcher(matcher);

    const int argc = argumentCount();
    if (argc != other->argumentCount())
        return false;
    for (int i = 0; i < argc; ++i) {
        Symbol *l = argumentAt(i);
        Symbol *r = other->argumentAt(i);
        if (! l->type().match(r->type(), matcher)) {
            if (!l->type()->asReferenceType() && !l->type()->asPointerType()
                    && !l->type()->asPointerToMemberType()
                    && !r->type()->asReferenceType() && !r->type()->asPointerType()
                    && !r->type()->asPointerToMemberType()) {
                FullySpecifiedType lType = l->type();
                FullySpecifiedType rType = r->type();
                lType.setConst(false);
                lType.setVolatile(false);
                rType.setConst(false);
                rType.setVolatile(false);
                if (lType.match(rType))
                    continue;
            }
            if (l->type().match(r->type(), &fallbackMatcher))
                continue;
            return false;
        }
    }
    return true;
}

void Function::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Function::match0(const Type *otherType, Matcher *matcher) const
{
    if (const Function *otherTy = otherType->asFunctionType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType Function::type() const
{
    FullySpecifiedType ty(const_cast<Function *>(this));
    ty.setConst(isConst());
    ty.setVolatile(isVolatile());
    ty.setStatic(isStatic());
    return ty;
}

bool Function::hasReturnType() const
{
    const FullySpecifiedType ty = returnType();
    return ty.isValid() || ty.isSigned() || ty.isUnsigned();
}

int Function::argumentCount() const
{
    const int memCnt = memberCount();
    if (memCnt > 0 && memberAt(0)->type()->asVoidType())
        return 0;

    // Definitions with function-try-blocks will have more than a block, and
    // arguments with a lambda as default argument will also have more blocks.
    int argc = 0;
    for (int it = 0; it < memCnt; ++it)
        if (memberAt(it)->asArgument())
            ++argc;
    return argc;
}

Symbol *Function::argumentAt(int index) const
{
    for (int it = 0, eit = memberCount(); it < eit; ++it) {
        if (Argument *arg = memberAt(it)->asArgument()) {
            if (index == 0)
                return arg;
            else
                --index;
        }
    }

    return nullptr;
}

bool Function::hasArguments() const
{
    int argc = argumentCount();
    return ! (argc == 0 || (argc == 1 && argumentAt(0)->type()->asVoidType()));
}

int Function::minimumArgumentCount() const
{
    int index = 0;

    for (int ei = argumentCount(); index < ei; ++index) {
        if (Argument *arg = argumentAt(index)->asArgument()) {
            if (arg->hasInitializer())
                break;
        }
    }

    return index;
}

void Function::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

bool Function::maybeValidPrototype(int actualArgumentCount) const
{
    const int argc = argumentCount();
    int minNumberArguments = 0;

    for (; minNumberArguments < argc; ++minNumberArguments) {
        Argument *arg = argumentAt(minNumberArguments)->asArgument();

        if (! arg)
            return false;

        if (arg->hasInitializer())
            break;
    }

    if (isVariadicTemplate())
        --minNumberArguments;

    if (actualArgumentCount < minNumberArguments) {
        // not enough arguments.
        return false;

    } else if (!isVariadic() && actualArgumentCount > argc) {
        // too many arguments.
        return false;
    }

    return true;
}


Block::Block(TranslationUnit *translationUnit, int sourceLocation)
    : Scope(translationUnit, sourceLocation, /*name = */ nullptr)
{ }

Block::Block(Clone *clone, Subst *subst, Block *original)
    : Scope(clone, subst, original)
{ }

FullySpecifiedType Block::type() const
{ return FullySpecifiedType(); }

void Block::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

Enum::Enum(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name)
    , _isScoped(false)
{ }

Enum::Enum(Clone *clone, Subst *subst, Enum *original)
    : Scope(clone, subst, original)
    , _isScoped(original->isScoped())
{ }

FullySpecifiedType Enum::type() const
{ return FullySpecifiedType(const_cast<Enum *>(this)); }


void Enum::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Enum::match0(const Type *otherType, Matcher *matcher) const
{
    if (const Enum *otherTy = otherType->asEnumType())
        return matcher->match(this, otherTy);

    return false;
}

void Enum::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

Template::Template(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name)
{ }

Template::Template(Clone *clone, Subst *subst, Template *original)
    : Scope(clone, subst, original)
{ }

int Template::templateParameterCount() const
{
    if (declaration() != nullptr)
        return memberCount() - 1;

    return 0;
}

Symbol *Template::declaration() const
{
    if (isEmpty())
        return nullptr;

    if (Symbol *s = memberAt(memberCount() - 1)) {
        if (s->asClass() || s->asForwardClassDeclaration() ||
            s->asTemplate() || s->asFunction() || s->asDeclaration())
            return s;
    }

    return nullptr;
}

FullySpecifiedType Template::type() const
{ return FullySpecifiedType(const_cast<Template *>(this)); }

void Template::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

void Template::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Template::match0(const Type *otherType, Matcher *matcher) const
{
    if (const Template *otherTy = otherType->asTemplateType())
        return matcher->match(this, otherTy);
    return false;
}

Namespace::Namespace(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name)
    , _isInline(false)
{ }

Namespace::Namespace(Clone *clone, Subst *subst, Namespace *original)
    : Scope(clone, subst, original)
    , _isInline(original->_isInline)
{ }

void Namespace::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Namespace::match0(const Type *otherType, Matcher *matcher) const
{
    if (const Namespace *otherTy = otherType->asNamespaceType())
        return matcher->match(this, otherTy);

    return false;
}

void Namespace::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

FullySpecifiedType Namespace::type() const
{ return FullySpecifiedType(const_cast<Namespace *>(this)); }

BaseClass::BaseClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name),
      _isVirtual(false)
{ }

BaseClass::BaseClass(Clone *clone, Subst *subst, BaseClass *original)
    : Symbol(clone, subst, original)
    , _isVirtual(original->_isVirtual)
    , _type(clone->type(original->_type, subst))
{ }

void BaseClass::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


ForwardClassDeclaration::ForwardClassDeclaration(TranslationUnit *translationUnit,
                                                 int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

ForwardClassDeclaration::ForwardClassDeclaration(Clone *clone, Subst *subst, ForwardClassDeclaration *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType ForwardClassDeclaration::type() const
{ return FullySpecifiedType(const_cast<ForwardClassDeclaration *>(this)); }

void ForwardClassDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ForwardClassDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ForwardClassDeclaration::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ForwardClassDeclaration *otherTy = otherType->asForwardClassDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

Class::Class(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name),
      _key(ClassKey)
{ }

Class::Class(Clone *clone, Subst *subst, Class *original)
    : Scope(clone, subst, original)
    , _key(original->_key)
{
    for (size_t i = 0; i < original->_baseClasses.size(); ++i)
        addBaseClass(clone->symbol(original->_baseClasses.at(i), subst)->asBaseClass());
}

void Class::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool Class::match0(const Type *otherType, Matcher *matcher) const
{
    if (const Class *otherTy = otherType->asClassType())
        return matcher->match(this, otherTy);

    return false;
}

int Class::baseClassCount() const
{ return int(_baseClasses.size()); }

BaseClass *Class::baseClassAt(int index) const
{ return _baseClasses.at(index); }

void Class::addBaseClass(BaseClass *baseClass)
{ _baseClasses.push_back(baseClass); }

FullySpecifiedType Class::type() const
{ return FullySpecifiedType(const_cast<Class *>(this)); }

void Class::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < int(_baseClasses.size()); ++i) {
            visitSymbol(_baseClasses.at(i), visitor);
        }
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}


QtPropertyDeclaration::QtPropertyDeclaration(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
    , _flags(NoFlags)
{ }

QtPropertyDeclaration::QtPropertyDeclaration(Clone *clone, Subst *subst, QtPropertyDeclaration *original)
    : Symbol(clone, subst, original)
    , _type(clone->type(original->_type, subst))
    , _flags(original->_flags)
{ }

void QtPropertyDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


QtEnum::QtEnum(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

QtEnum::QtEnum(Clone *clone, Subst *subst, QtEnum *original)
    : Symbol(clone, subst, original)
{ }

void QtEnum::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }


ObjCBaseClass::ObjCBaseClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

ObjCBaseClass::ObjCBaseClass(Clone *clone, Subst *subst, ObjCBaseClass *original)
    : Symbol(clone, subst, original)
{ }


FullySpecifiedType ObjCBaseClass::type() const
{ return FullySpecifiedType(); }

void ObjCBaseClass::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

ObjCBaseProtocol::ObjCBaseProtocol(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Symbol(translationUnit, sourceLocation, name)
{ }

ObjCBaseProtocol::ObjCBaseProtocol(Clone *clone, Subst *subst, ObjCBaseProtocol *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType ObjCBaseProtocol::type() const
{ return FullySpecifiedType(); }

void ObjCBaseProtocol::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

ObjCClass::ObjCClass(TranslationUnit *translationUnit, int sourceLocation, const Name *name):
    Scope(translationUnit, sourceLocation, name),
    _categoryName(nullptr),
    _baseClass(nullptr),
    _isInterface(false)
{ }

ObjCClass::ObjCClass(Clone *clone, Subst *subst, ObjCClass *original)
    : Scope(clone, subst, original)
    , _categoryName(clone->name(original->_categoryName, subst))
    , _baseClass(nullptr)
    , _isInterface(original->_isInterface)
{
    if (original->_baseClass)
        _baseClass = clone->symbol(original->_baseClass, subst)->asObjCBaseClass();
    for (size_t i = 0; i < original->_protocols.size(); ++i)
        addProtocol(clone->symbol(original->_protocols.at(i), subst)->asObjCBaseProtocol());
}

int ObjCClass::protocolCount() const
{ return int(_protocols.size()); }

ObjCBaseProtocol *ObjCClass::protocolAt(int index) const
{ return _protocols.at(index); }

void ObjCClass::addProtocol(ObjCBaseProtocol *protocol)
{ _protocols.push_back(protocol); }

FullySpecifiedType ObjCClass::type() const
{ return FullySpecifiedType(const_cast<ObjCClass *>(this)); }

void ObjCClass::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        if (_baseClass)
            visitSymbol(_baseClass, visitor);

        for (int i = 0; i < int(_protocols.size()); ++i)
            visitSymbol(_protocols.at(i), visitor);

        for (int i = 0; i < memberCount(); ++i)
            visitSymbol(memberAt(i), visitor);
    }
}

void ObjCClass::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCClass::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ObjCClass *otherTy = otherType->asObjCClassType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCProtocol::ObjCProtocol(TranslationUnit *translationUnit, int sourceLocation, const Name *name):
        Scope(translationUnit, sourceLocation, name)
{
}

ObjCProtocol::ObjCProtocol(Clone *clone, Subst *subst, ObjCProtocol *original)
    : Scope(clone, subst, original)
{
    for (size_t i = 0; i < original->_protocols.size(); ++i)
        addProtocol(clone->symbol(original->_protocols.at(i), subst)->asObjCBaseProtocol());
}

int ObjCProtocol::protocolCount() const
{ return int(_protocols.size()); }

ObjCBaseProtocol *ObjCProtocol::protocolAt(int index) const
{ return _protocols.at(index); }

void ObjCProtocol::addProtocol(ObjCBaseProtocol *protocol)
{ _protocols.push_back(protocol); }

FullySpecifiedType ObjCProtocol::type() const
{ return FullySpecifiedType(const_cast<ObjCProtocol *>(this)); }

void ObjCProtocol::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < int(_protocols.size()); ++i)
            visitSymbol(_protocols.at(i), visitor);
    }
}

void ObjCProtocol::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCProtocol::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ObjCProtocol *otherTy = otherType->asObjCProtocolType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCForwardClassDeclaration::ObjCForwardClassDeclaration(TranslationUnit *translationUnit, int sourceLocation,
                                                         const Name *name):
        Symbol(translationUnit, sourceLocation, name)
{
}

ObjCForwardClassDeclaration::ObjCForwardClassDeclaration(Clone *clone, Subst *subst, ObjCForwardClassDeclaration *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType ObjCForwardClassDeclaration::type() const
{ return FullySpecifiedType(); }

void ObjCForwardClassDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ObjCForwardClassDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCForwardClassDeclaration::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ObjCForwardClassDeclaration *otherTy = otherType->asObjCForwardClassDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCForwardProtocolDeclaration::ObjCForwardProtocolDeclaration(TranslationUnit *translationUnit, int sourceLocation,
                                                               const Name *name):
        Symbol(translationUnit, sourceLocation, name)
{
}

ObjCForwardProtocolDeclaration::ObjCForwardProtocolDeclaration(Clone *clone, Subst *subst, ObjCForwardProtocolDeclaration *original)
    : Symbol(clone, subst, original)
{ }

FullySpecifiedType ObjCForwardProtocolDeclaration::type() const
{ return FullySpecifiedType(); }

void ObjCForwardProtocolDeclaration::visitSymbol0(SymbolVisitor *visitor)
{ visitor->visit(this); }

void ObjCForwardProtocolDeclaration::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCForwardProtocolDeclaration::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ObjCForwardProtocolDeclaration *otherTy = otherType->asObjCForwardProtocolDeclarationType())
        return matcher->match(this, otherTy);

    return false;
}

ObjCMethod::ObjCMethod(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : Scope(translationUnit, sourceLocation, name),
     _flags(0)
{ }

ObjCMethod::ObjCMethod(Clone *clone, Subst *subst, ObjCMethod *original)
    : Scope(clone, subst, original)
    , _returnType(clone->type(original->_returnType, subst))
    , _flags(original->_flags)
{ }

void ObjCMethod::accept0(TypeVisitor *visitor)
{ visitor->visit(this); }

bool ObjCMethod::match0(const Type *otherType, Matcher *matcher) const
{
    if (const ObjCMethod *otherTy = otherType->asObjCMethodType())
        return matcher->match(this, otherTy);

    return false;
}

FullySpecifiedType ObjCMethod::type() const
{ return FullySpecifiedType(const_cast<ObjCMethod *>(this)); }

bool ObjCMethod::hasReturnType() const
{
    const FullySpecifiedType ty = returnType();
    return ty.isValid() || ty.isSigned() || ty.isUnsigned();
}

int ObjCMethod::argumentCount() const
{
    const int c = memberCount();
    if (c > 0 && memberAt(c - 1)->asBlock())
        return c - 1;
    return c;
}

Symbol *ObjCMethod::argumentAt(int index) const
{
    return memberAt(index);
}

bool ObjCMethod::hasArguments() const
{
    return ! (argumentCount() == 0 ||
              (argumentCount() == 1 && argumentAt(0)->type()->asVoidType()));
}

void ObjCMethod::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
        for (int i = 0; i < memberCount(); ++i) {
            visitSymbol(memberAt(i), visitor);
        }
    }
}

ObjCPropertyDeclaration::ObjCPropertyDeclaration(TranslationUnit *translationUnit,
                                                 int sourceLocation,
                                                 const Name *name):
    Symbol(translationUnit, sourceLocation, name),
    _getterName(nullptr),
    _setterName(nullptr),
    _propertyAttributes(None)
{}

ObjCPropertyDeclaration::ObjCPropertyDeclaration(Clone *clone, Subst *subst, ObjCPropertyDeclaration *original)
    : Symbol(clone, subst, original)
    , _getterName(clone->name(original->_getterName, subst))
    , _setterName(clone->name(original->_setterName, subst))
    , _type(clone->type(original->_type, subst))
    , _propertyAttributes(original->_propertyAttributes)
{ }

void ObjCPropertyDeclaration::visitSymbol0(SymbolVisitor *visitor)
{
    if (visitor->visit(this)) {
    }
}
