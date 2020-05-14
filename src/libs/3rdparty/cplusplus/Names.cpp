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

#include "Names.h"
#include "Matcher.h"
#include "NameVisitor.h"
#include "Literals.h"
#include <algorithm>
#include <cstring>

using namespace CPlusPlus;

QualifiedNameId::~QualifiedNameId()
{ }

void QualifiedNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool QualifiedNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const QualifiedNameId *name = otherName->asQualifiedNameId())
        return matcher->match(this, name);
    return false;
}

const Identifier *QualifiedNameId::identifier() const
{
    if (const Name *u = name())
        return u->identifier();

    return nullptr;
}

const Name *QualifiedNameId::base() const
{ return _base; }

const Name *QualifiedNameId::name() const
{ return _name; }

DestructorNameId::DestructorNameId(const Name *name)
    : _name(name)
{ }

DestructorNameId::~DestructorNameId()
{ }

void DestructorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool DestructorNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const DestructorNameId *name = otherName->asDestructorNameId())
        return matcher->match(this, name);
    return false;
}

const Name *DestructorNameId::name() const
{ return _name; }

const Identifier *DestructorNameId::identifier() const
{ return _name->identifier(); }

TemplateNameId::~TemplateNameId()
{ }

void TemplateNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool TemplateNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const TemplateNameId *other = otherName->asTemplateNameId())
        return matcher->match(this, other);
    return false;
}

const Identifier *TemplateNameId::identifier() const
{ return _identifier; }

bool TemplateArgument::match(const TemplateArgument &otherTy, Matcher *matcher) const
{
    if (_numericLiteral != otherTy._numericLiteral)
        return false;
    return type().match(otherTy.type(), matcher);
}

int TemplateNameId::templateArgumentCount() const
{ return int(_templateArguments.size()); }

const TemplateArgument &TemplateNameId::templateArgumentAt(int index) const
{ return _templateArguments[index]; }

bool TemplateNameId::Compare::operator()(const TemplateNameId *name,
                                         const TemplateNameId *other) const
{
    if (name == nullptr)
        return other != nullptr;
    if (other == nullptr)
        return false;
    if (name == other)
        return false;

    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();

    if (id == nullptr)
        return otherId != nullptr;
    if (otherId == nullptr)
        return false;

    const int c = std::strcmp(id->chars(), otherId->chars());
    if (c == 0) {
        // we have to differentiate TemplateNameId with respect to specialization or instantiation
        if (name->isSpecialization() == other->isSpecialization()) {
            return std::lexicographical_compare(name->firstTemplateArgument(),
                                                name->lastTemplateArgument(),
                                                other->firstTemplateArgument(),
                                                other->lastTemplateArgument());
        } else {
            return name->isSpecialization();
        }
    }

    return c < 0;
}

OperatorNameId::OperatorNameId(Kind kind)
    : _kind(kind)
{ }

OperatorNameId::~OperatorNameId()
{ }

void OperatorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool OperatorNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const OperatorNameId *name = otherName->asOperatorNameId())
        return matcher->match(this, name);
    return false;
}

OperatorNameId::Kind OperatorNameId::kind() const
{ return _kind; }

const Identifier *OperatorNameId::identifier() const
{ return nullptr; }

ConversionNameId::ConversionNameId(const FullySpecifiedType &type)
    : _type(type)
{ }

ConversionNameId::~ConversionNameId()
{ }

void ConversionNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool ConversionNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const ConversionNameId *name = otherName->asConversionNameId())
        return matcher->match(this, name);
    return false;
}

FullySpecifiedType ConversionNameId::type() const
{ return _type; }

const Identifier *ConversionNameId::identifier() const
{ return nullptr; }

SelectorNameId::~SelectorNameId()
{ }

void SelectorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool SelectorNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const SelectorNameId *name = otherName->asSelectorNameId())
        return matcher->match(this, name);
    return false;
}

const Identifier *SelectorNameId::identifier() const
{
    if (_names.empty())
        return nullptr;

    return nameAt(0)->identifier();
}

int SelectorNameId::nameCount() const
{ return int(_names.size()); }

const Name *SelectorNameId::nameAt(int index) const
{ return _names[index]; }

bool SelectorNameId::hasArguments() const
{ return _hasArguments; }

AnonymousNameId::AnonymousNameId(int classTokenIndex)
    : _classTokenIndex(classTokenIndex)
{ }

AnonymousNameId::~AnonymousNameId()
{ }

int AnonymousNameId::classTokenIndex() const
{
    return _classTokenIndex;
}

void AnonymousNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

bool AnonymousNameId::match0(const Name *otherName, Matcher *matcher) const
{
    if (const AnonymousNameId *id = otherName->asAnonymousNameId())
        return matcher->match(this, id);
    return false;
}

const Identifier *AnonymousNameId::identifier() const
{ return nullptr; }
