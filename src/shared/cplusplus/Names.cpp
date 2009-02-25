/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
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
#include <algorithm>

CPLUSPLUS_BEGIN_NAMESPACE

QualifiedNameId::QualifiedNameId(Name *const names[],
                                 unsigned nameCount,
                                 bool isGlobal)
    : _names(0),
      _nameCount(nameCount),
      _isGlobal(isGlobal)
{
    if (_nameCount) {
        _names = new Name *[_nameCount];
        std::copy(&names[0], &names[nameCount], _names);
    }
}

QualifiedNameId::~QualifiedNameId()
{ delete[] _names; }

void QualifiedNameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

unsigned QualifiedNameId::nameCount() const
{ return _nameCount; }

Name *QualifiedNameId::nameAt(unsigned index) const
{ return _names[index]; }

Name *const *QualifiedNameId::names() const
{ return _names; }

bool QualifiedNameId::isGlobal() const
{ return _isGlobal; }

Name *QualifiedNameId::unqualifiedNameId() const
{
    if (! _nameCount)
        return 0;

    return _names[_nameCount - 1];
}

bool QualifiedNameId::isEqualTo(const Name *other) const
{
    const QualifiedNameId *q = other->asQualifiedNameId();
    if (! q)
        return false;
    else if (isGlobal() != q->isGlobal())
        return false;
    else {
        const unsigned count = nameCount();
        if (count != q->nameCount())
            return false;
        for (unsigned i = 0; i < count; ++i) {
            Name *l = nameAt(i);
            Name *r = q->nameAt(i);
            if (! l->isEqualTo(r))
                return false;
        }
    }
    return true;
}

NameId::NameId(Identifier *identifier)
    : _identifier(identifier)
{ }

NameId::~NameId()
{ }

void NameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

Identifier *NameId::identifier() const
{ return _identifier; }

bool NameId::isEqualTo(const Name *other) const
{
    const NameId *nameId = other->asNameId();
    if (! nameId)
        return false;
    Identifier *l = identifier();
    Identifier *r = nameId->identifier();
    return l->isEqualTo(r);
}

DestructorNameId::DestructorNameId(Identifier *identifier)
    : _identifier(identifier)
{ }

DestructorNameId::~DestructorNameId()
{ }

void DestructorNameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

Identifier *DestructorNameId::identifier() const
{ return _identifier; }

bool DestructorNameId::isEqualTo(const Name *other) const
{
    const DestructorNameId *d = other->asDestructorNameId();
    if (! d)
        return false;
    Identifier *l = identifier();
    Identifier *r = d->identifier();
    return l->isEqualTo(r);
}

TemplateNameId::TemplateNameId(Identifier *identifier,
        const FullySpecifiedType templateArguments[],
        unsigned templateArgumentCount)
    : _identifier(identifier),
      _templateArguments(0),
      _templateArgumentCount(templateArgumentCount)
{
    if (_templateArgumentCount) {
        _templateArguments = new FullySpecifiedType[_templateArgumentCount];
        std::copy(&templateArguments[0], &templateArguments[_templateArgumentCount],
                  _templateArguments);
    }
}

TemplateNameId::~TemplateNameId()
{ delete[] _templateArguments; }

void TemplateNameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

Identifier *TemplateNameId::identifier() const
{ return _identifier; }

unsigned TemplateNameId::templateArgumentCount() const
{ return _templateArgumentCount; }

const FullySpecifiedType &TemplateNameId::templateArgumentAt(unsigned index) const
{ return _templateArguments[index]; }

const FullySpecifiedType *TemplateNameId::templateArguments() const
{ return _templateArguments; }

bool TemplateNameId::isEqualTo(const Name *other) const
{
    const TemplateNameId *t = other->asTemplateNameId();
    if (! t)
        return false;
    Identifier *l = identifier();
    Identifier *r = t->identifier();
    if (! l->isEqualTo(r))
        return false;
    if (_templateArgumentCount != t->_templateArgumentCount)
        return false;
    for (unsigned i = 0; i < _templateArgumentCount; ++i) {
        const FullySpecifiedType &l = _templateArguments[i];
        const FullySpecifiedType &r = t->_templateArguments[i];
        if (! l.isEqualTo(r))
            return false;
    }
    return true;
}

OperatorNameId::OperatorNameId(int kind)
    : _kind(kind)
{ }

OperatorNameId::~OperatorNameId()
{ }

void OperatorNameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

int OperatorNameId::kind() const
{ return _kind; }

bool OperatorNameId::isEqualTo(const Name *other) const
{
    const OperatorNameId *o = other->asOperatorNameId();
    if (! o)
        return false;
    return _kind == o->kind();
}

ConversionNameId::ConversionNameId(FullySpecifiedType type)
    : _type(type)
{ }

ConversionNameId::~ConversionNameId()
{ }

void ConversionNameId::accept0(NameVisitor *visitor)
{ visitor->visit(this); }

FullySpecifiedType ConversionNameId::type() const
{ return _type; }

bool ConversionNameId::isEqualTo(const Name *other) const
{
    const ConversionNameId *c = other->asConversionNameId();
    if (! c)
        return false;
    return _type.isEqualTo(c->type());
}


CPLUSPLUS_END_NAMESPACE
