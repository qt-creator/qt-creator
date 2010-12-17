/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/
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

DestructorNameId::DestructorNameId(const Identifier *identifier)
    : _identifier(identifier)
{ }

DestructorNameId::~DestructorNameId()
{ }

void DestructorNameId::accept0(NameVisitor *visitor) const
{ visitor->visit(this); }

const Identifier *DestructorNameId::identifier() const
{ return _identifier; }

bool DestructorNameId::isEqualTo(const Name *other) const
{
    if (other) {
        const DestructorNameId *d = other->asDestructorNameId();
        if (! d)
            return false;
        const Identifier *l = identifier();
        const Identifier *r = d->identifier();
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

