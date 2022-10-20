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

#pragma once

#include "CPlusPlusForwardDeclarations.h"
#include "Name.h"
#include "FullySpecifiedType.h"
#include <functional>
#include <vector>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT QualifiedNameId: public Name
{
public:
    QualifiedNameId(const Name *base, const Name *name)
        : _base(base), _name(name) {}

    virtual ~QualifiedNameId();

    const Identifier *identifier() const override;

    const Name *base() const;
    const Name *name() const;

    const QualifiedNameId *asQualifiedNameId() const override
    { return this; }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    const Name *_base;
    const Name *_name;
};

class CPLUSPLUS_EXPORT DestructorNameId: public Name
{
public:
    DestructorNameId(const Name *name);
    virtual ~DestructorNameId();

    const Name *name() const;

    const Identifier *identifier() const override;

    const DestructorNameId *asDestructorNameId() const override
    { return this; }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    const Name *_name;
};

class CPLUSPLUS_EXPORT TemplateArgument
{
public:
    TemplateArgument()
        : _expressionTy(nullptr)
        , _numericLiteral(nullptr)
    {}

    TemplateArgument(const FullySpecifiedType &type, const NumericLiteral *numericLiteral = nullptr)
        : _expressionTy(type)
        , _numericLiteral(numericLiteral)
    {}

    bool hasType() const { return _expressionTy.isValid(); }

    bool hasNumericLiteral() const { return _numericLiteral != nullptr; }

    const FullySpecifiedType &type() const { return _expressionTy; }
    FullySpecifiedType &type() { return _expressionTy; }

    const NumericLiteral *numericLiteral() const { return _numericLiteral; }
    void setNumericLiteral(const NumericLiteral *l) { _numericLiteral = l; }

    bool operator==(const TemplateArgument &other) const
    {
        return _expressionTy == other._expressionTy && _numericLiteral == other._numericLiteral;
    }
    bool operator!=(const TemplateArgument &other) const
    {
        return _expressionTy != other._expressionTy || _numericLiteral != other._numericLiteral;
    }
    bool operator<(const TemplateArgument &other) const
    {
        if (_expressionTy == other._expressionTy) {
            return _numericLiteral < other._numericLiteral;
        }
        return _expressionTy < other._expressionTy;
    }

    bool match(const TemplateArgument &otherTy, Matcher *matcher = nullptr) const;

    size_t hash() const
    {
        return _expressionTy.hash() ^ std::hash<const NumericLiteral *>()(_numericLiteral);
    }

private:
    FullySpecifiedType _expressionTy;
    const NumericLiteral *_numericLiteral = nullptr;
};

class CPLUSPLUS_EXPORT TemplateNameId: public Name
{
public:
    template <typename Iterator>
    TemplateNameId(const Identifier *identifier, bool isSpecialization, Iterator first,
                   Iterator last)
        : _identifier(identifier)
        , _templateArguments(first, last)
        , _isSpecialization(isSpecialization) {}

    virtual ~TemplateNameId();

    const Identifier *identifier() const override;

    // ### find a better name
    int templateArgumentCount() const;
    const TemplateArgument &templateArgumentAt(int index) const;

    const TemplateNameId *asTemplateNameId() const override
    { return this; }

    typedef std::vector<TemplateArgument>::const_iterator TemplateArgumentIterator;

    TemplateArgumentIterator firstTemplateArgument() const { return _templateArguments.begin(); }
    TemplateArgumentIterator lastTemplateArgument() const { return _templateArguments.end(); }
    bool isSpecialization() const { return _isSpecialization; }

    // Comparator needed to distinguish between two different TemplateNameId(e.g.:used in std::unordered_map)
    struct Equals {
        bool operator()(const TemplateNameId *name, const TemplateNameId *other) const;
    };
    struct Hash {
        size_t operator()(const TemplateNameId *name) const;
    };

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    const Identifier *_identifier;
    std::vector<TemplateArgument> _templateArguments;
    // now TemplateNameId can be a specialization or an instantiation
    bool _isSpecialization;
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
        ()   []   <=>
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
        ArrayAccessOp,
        SpaceShipOp
    };

public:
    OperatorNameId(Kind kind);
    virtual ~OperatorNameId();

    Kind kind() const;

    const Identifier *identifier() const override { return nullptr; }
    const OperatorNameId *asOperatorNameId() const override { return this; }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    Kind _kind;
};

class CPLUSPLUS_EXPORT ConversionNameId: public Name
{
public:
    ConversionNameId(const FullySpecifiedType &type);
    virtual ~ConversionNameId();

    FullySpecifiedType type() const { return _type; }
    const Identifier *identifier() const override { return nullptr; }
    const ConversionNameId *asConversionNameId() const override { return this; }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT SelectorNameId: public Name
{
public:
    template <typename Iterator>
    SelectorNameId(Iterator first, Iterator last, bool hasArguments)
        : _names(first, last), _hasArguments(hasArguments) {}

    virtual ~SelectorNameId();

    const Identifier *identifier() const override;

    int nameCount() const;
    const Name *nameAt(int index) const;
    bool hasArguments() const;

    const SelectorNameId *asSelectorNameId() const override
    { return this; }

    typedef std::vector<const Name *>::const_iterator NameIterator;

    NameIterator firstName() const { return _names.begin(); }
    NameIterator lastName() const { return _names.end(); }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    std::vector<const Name *> _names;
    bool _hasArguments;
};

class CPLUSPLUS_EXPORT AnonymousNameId final : public Name
{
public:
    AnonymousNameId(int classTokenIndex);
    virtual ~AnonymousNameId();

    int classTokenIndex() const;

    const Identifier *identifier() const override { return nullptr; }

    const AnonymousNameId *asAnonymousNameId() const override { return this; }

protected:
    void accept0(NameVisitor *visitor) const override;
    bool match0(const Name *otherName, Matcher *matcher) const override;

private:
    int _classTokenIndex;
};



} // namespace CPlusPlus
