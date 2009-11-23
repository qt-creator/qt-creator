/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

namespace CPlusPlus {

class CPLUSPLUS_EXPORT QualifiedNameId: public Name
{
public:
    QualifiedNameId(Name *const names[],
                    unsigned nameCount,
                    bool isGlobal = false);
    virtual ~QualifiedNameId();

    virtual Identifier *identifier() const;

    unsigned nameCount() const;
    Name *nameAt(unsigned index) const;
    Name *const *names() const;
    Name *unqualifiedNameId() const;

    bool isGlobal() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const QualifiedNameId *asQualifiedNameId() const
    { return this; }

    virtual QualifiedNameId *asQualifiedNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    Name **_names;
    unsigned _nameCount;
    bool _isGlobal;
};

class CPLUSPLUS_EXPORT NameId: public Name
{
public:
    NameId(Identifier *identifier);
    virtual ~NameId();

    virtual Identifier *identifier() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const NameId *asNameId() const
    { return this; }

    virtual NameId *asNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    Identifier *_identifier;
};

class CPLUSPLUS_EXPORT DestructorNameId: public Name
{
public:
    DestructorNameId(Identifier *identifier);
    virtual ~DestructorNameId();

    virtual Identifier *identifier() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const DestructorNameId *asDestructorNameId() const
    { return this; }

    virtual DestructorNameId *asDestructorNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    Identifier *_identifier;
};

class CPLUSPLUS_EXPORT TemplateNameId: public Name
{
public:
    TemplateNameId(Identifier *identifier,
                   const FullySpecifiedType templateArguments[],
                   unsigned templateArgumentCount);
    virtual ~TemplateNameId();

    virtual Identifier *identifier() const;

    // ### find a better name
    unsigned templateArgumentCount() const;
    const FullySpecifiedType &templateArgumentAt(unsigned index) const;
    const FullySpecifiedType *templateArguments() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const TemplateNameId *asTemplateNameId() const
    { return this; }

    virtual TemplateNameId *asTemplateNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    Identifier *_identifier;
    FullySpecifiedType *_templateArguments;
    unsigned _templateArgumentCount;
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
    OperatorNameId(int kind);
    virtual ~OperatorNameId();

    int kind() const;

    virtual Identifier *identifier() const;
    virtual bool isEqualTo(const Name *other) const;

    virtual const OperatorNameId *asOperatorNameId() const
    { return this; }

    virtual OperatorNameId *asOperatorNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    int _kind;
};

class CPLUSPLUS_EXPORT ConversionNameId: public Name
{
public:
    ConversionNameId(const FullySpecifiedType &type);
    virtual ~ConversionNameId();

    FullySpecifiedType type() const;

    virtual Identifier *identifier() const;
    virtual bool isEqualTo(const Name *other) const;

    virtual const ConversionNameId *asConversionNameId() const
    { return this; }

    virtual ConversionNameId *asConversionNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT SelectorNameId: public Name
{
public:
    SelectorNameId(Name *const names[],
                   unsigned nameCount,
                   bool hasArguments);
    virtual ~SelectorNameId();

    virtual Identifier *identifier() const;

    unsigned nameCount() const;
    Name *nameAt(unsigned index) const;
    Name *const *names() const;

    bool hasArguments() const;

    virtual bool isEqualTo(const Name *other) const;

    virtual const SelectorNameId *asSelectorNameId() const
    { return this; }

    virtual SelectorNameId *asSelectorNameId()
    { return this; }

protected:
    virtual void accept0(NameVisitor *visitor);

private:
    Name **_names;
    unsigned _nameCount;
    bool _hasArguments;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_NAMES_H
