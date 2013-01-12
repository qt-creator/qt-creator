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
#include "NameVisitor.h"
#include "Literals.h"
#include <cstring>
#include <cassert>
#include <algorithm>

using namespace CPlusPlus;

QualifiedNameId::~QualifiedNameId()
{ }

void QualifiedNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

const Identifier *QualifiedNameId::identifier() const
{
    if (const Name *u = name())
        return u->identifier();

    return 0;
}

const Name *QualifiedNameId::base() const
{ return _base; }

const Name *QualifiedNameId::name() const
{ return _name; }

bool QualifiedNameId::isEqualTo(const Name *other) const
{
    if (other) {
        if (const QualifiedNameId *q = other->asQualifiedNameId()) {
            if (_base == q->_base || (_base && _base->isEqualTo(q->_base))) {
                if (_name == q->_name || (_name && _name->isEqualTo(q->_name))) {
                    return true;
                }
            }
        }
    }

    return false;
}

DestructorNameId::DestructorNameId(const Name *name)
    : _name(name)
{ }

DestructorNameId::~DestructorNameId()
{ }

void DestructorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

const Name *DestructorNameId::name() const
{ return _name; }

const Identifier *DestructorNameId::identifier() const
{ return _name->identifier(); }

bool DestructorNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const DestructorNameId *d = other->asDestructorNameId();
        if (! d)
            return false;
        const Name *l = name();
        const Name *r = d->name();
        return l->isEqualTo(r);
    }
    return false;
}

TemplateNameId::~TemplateNameId()
{ }

void TemplateNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

const Identifier *TemplateNameId::identifier() const
{ return _identifier; }

unsigned TemplateNameId::templateArgumentCount() const
{ return _templateArguments.size(); }

const FullySpecifiedType &TemplateNameId::templateArgumentAt(unsigned index) const
{ return _templateArguments[index]; }

bool TemplateNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const TemplateNameId *t = other->asTemplateNameId();
        if (! t)
            return false;
        const Identifier *l = identifier();
        const Identifier *r = t->identifier();
        if (! l->isEqualTo(r))
            return false;
        if (templateArgumentCount() != t->templateArgumentCount())
            return false;
        for (unsigned i = 0; i < templateArgumentCount(); ++i) {
            const FullySpecifiedType &l = _templateArguments[i];
            const FullySpecifiedType &r = t->_templateArguments[i];
            if (! l.isEqualTo(r))
                return false;
        }
    }
    return true;
}

bool TemplateNameId::Compare::operator()(const TemplateNameId *name,
                                         const TemplateNameId *other) const
{
    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();

    if (id == otherId) {
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

    return id < otherId;
}

OperatorNameId::OperatorNameId(Kind kind)
    : _kind(kind)
{ }

OperatorNameId::~OperatorNameId()
{ }

void OperatorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

OperatorNameId::Kind OperatorNameId::kind() const
{ return _kind; }

const Identifier *OperatorNameId::identifier() const
{ return 0; }

bool OperatorNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const OperatorNameId *o = other->asOperatorNameId();
        if (! o)
            return false;
        return _kind == o->kind();
    }
    return false;
}

ConversionNameId::ConversionNameId(const FullySpecifiedType &type)
    : _type(type)
{ }

ConversionNameId::~ConversionNameId()
{ }

void ConversionNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

FullySpecifiedType ConversionNameId::type() const
{ return _type; }

const Identifier *ConversionNameId::identifier() const
{ return 0; }

bool ConversionNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const ConversionNameId *c = other->asConversionNameId();
        if (! c)
            return false;
        return _type.isEqualTo(c->type());
    }
    return false;
}

SelectorNameId::~SelectorNameId()
{ }

void SelectorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

const Identifier *SelectorNameId::identifier() const
{
    if (_names.empty())
        return 0;

    return nameAt(0)->identifier();
}

unsigned SelectorNameId::nameCount() const
{ return _names.size(); }

const Name *SelectorNameId::nameAt(unsigned index) const
{ return _names[index]; }

bool SelectorNameId::hasArguments() const
{ return _hasArguments; }

bool SelectorNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const SelectorNameId *q = other->asSelectorNameId();
        if (! q)
            return false;
        else if (hasArguments() != q->hasArguments())
            return false;
        else {
            const unsigned count = nameCount();
            if (count != q->nameCount())
                return false;
            for (unsigned i = 0; i < count; ++i) {
                const Name *l = nameAt(i);
                const Name *r = q->nameAt(i);
                if (! l->isEqualTo(r))
                    return false;
            }
        }
    }
    return true;
}

