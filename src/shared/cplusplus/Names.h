/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef CPLUSPLUS_NAMES_H
#define CPLUSPLUS_NAMES_H

#include "CPlusPlusForwardDeclarations.h"
#include "Name.h"
#include "FullySpecifiedType.h"
#include <vector>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT QualifiedNameId: public Name
{
public:
    QualifiedNameId(const Name *base, const Name *name)
        : _base(base), _name(name) {}

    virtual ~QualifiedNameId();

    virtual const Identifier *identifier() const;

    const Name *base() const;
    const Name *name() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const QualifiedNameId *asQualifiedNameId() const
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    const Name *_base;
    const Name *_name;
};

class CPLUSPLUS_EXPORT DestructorNameId: public Name
{
public:
    DestructorNameId(const Identifier *identifier);
    virtual ~DestructorNameId();

    virtual const Identifier *identifier() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const DestructorNameId *asDestructorNameId() const
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    const Identifier *_identifier;
};

class CPLUSPLUS_EXPORT TemplateNameId: public Name
{
public:
    template <typename _Iterator>
    TemplateNameId(const Identifier *identifier, _Iterator first, _Iterator last)
        : _identifier(identifier), _templateArguments(first, last) {}

    virtual ~TemplateNameId();

    virtual const Identifier *identifier() const;

    // ### find a better name
    unsigned templateArgumentCount() const;
    const FullySpecifiedType &templateArgumentAt(unsigned index) const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const TemplateNameId *asTemplateNameId() const
    { return this; }

    typedef std::vector<FullySpecifiedType>::const_iterator TemplateArgumentIterator;

    TemplateArgumentIterator firstTemplateArgument() const { return _templateArguments.begin(); }
    TemplateArgumentIterator lastTemplateArgument() const { return _templateArguments.end(); }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    const Identifier *_identifier;
    std::vector<FullySpecifiedType> _templateArguments;
};

class CPLUSPLUS_EXPORT OperatorNameId: public Name
{
public:
    /*
        new  delete    new[]     delete[]
        +    -    *    /    %    ^    &    |    ~
        !    =    <    >    +=   -=   *=   /=   %=
        ^=   &=   |=   <<   >>   >>=  <<=  ==   !=
        <=   >=   &&   ||   ++   --   ,    ->*  ->
        ()   []
     */
    enum Kind {
        InvalidOp,
        NewOp,
        DeleteOp,
        NewArrayOp,
        DeleteArrayOp,
        PlusOp,
        MinusOp,
        StarOp,
        SlashOp,
        PercentOp,
        CaretOp,
        AmpOp,
        PipeOp,
        TildeOp,
        ExclaimOp,
        EqualOp,
        LessOp,
        GreaterOp,
        PlusEqualOp,
        MinusEqualOp,
        StarEqualOp,
        SlashEqualOp,
        PercentEqualOp,
        CaretEqualOp,
        AmpEqualOp,
        PipeEqualOp,
        LessLessOp,
        GreaterGreaterOp,
        LessLessEqualOp,
        GreaterGreaterEqualOp,
        EqualEqualOp,
        ExclaimEqualOp,
        LessEqualOp,
        GreaterEqualOp,
        AmpAmpOp,
        PipePipeOp,
        PlusPlusOp,
        MinusMinusOp,
        CommaOp,
        ArrowStarOp,
        ArrowOp,
        FunctionCallOp,
        ArrayAccessOp
    };

public:
    OperatorNameId(Kind kind);
    virtual ~OperatorNameId();

    Kind kind() const;

    virtual const Identifier *identifier() const;
    virtual bool isEqualTo(const Name *other) const;

    virtual const OperatorNameId *asOperatorNameId() const
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    Kind _kind;
};

class CPLUSPLUS_EXPORT ConversionNameId: public Name
{
public:
    ConversionNameId(const FullySpecifiedType &type);
    virtual ~ConversionNameId();

    FullySpecifiedType type() const;

    virtual const Identifier *identifier() const;
    virtual bool isEqualTo(const Name *other) const;

    virtual const ConversionNameId *asConversionNameId() const
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT SelectorNameId: public Name
{
public:
    template <typename _Iterator>
    SelectorNameId(_Iterator first, _Iterator last, bool hasArguments)
        : _names(first, last), _hasArguments(hasArguments) {}

    virtual ~SelectorNameId();

    virtual const Identifier *identifier() const;

    unsigned nameCount() const;
    const Name *nameAt(unsigned index) const;
    bool hasArguments() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const SelectorNameId *asSelectorNameId() const
    { return this; }

    typedef std::vector<const Name *>::const_iterator NameIterator;

    NameIterator firstName() const { return _names.begin(); }
    NameIterator lastName() const { return _names.end(); }

protected:
    virtual void accept0(NameVisitor *visitor) const;

private:
    std::vector<const Name *> _names;
    bool _hasArguments;
};

} // end of namespace CPlusPlus

#endif // CPLUSPLUS_NAMES_H
