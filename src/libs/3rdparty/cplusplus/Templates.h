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
#include "TypeVisitor.h"
#include "FullySpecifiedType.h"
#include "Name.h"
#include "NameVisitor.h"
#include "SymbolVisitor.h"

#include <map>
#include <unordered_map>
#include <utility>

namespace CPlusPlus {

class Clone;

class CPLUSPLUS_EXPORT Subst
{
    Subst(const Subst &other);
    Subst &operator = (const Subst &other);

public:
    Subst(Control *control, Subst *previous = nullptr)
        : _control(control)
        , _previous(previous)
    { }

    Control *control() const { return _control; }
    Subst *previous() const { return _previous; }

    FullySpecifiedType apply(const Name *name) const;

    void bind(const Name *name, const FullySpecifiedType &ty)
    { _map.insert(std::make_pair(name, ty)); }

    FullySpecifiedType &operator[](const Name *name) { return _map[name]; }

    bool contains(const Name *name) const { return _map.find(name) != _map.end(); }

private:
    Control *_control;
    Subst *_previous;
    std::unordered_map<const Name *, FullySpecifiedType, Name::Hash, Name::Equals> _map;
};

class CPLUSPLUS_EXPORT CloneType: protected TypeVisitor
{
public:
    CloneType(Clone *clone);

    FullySpecifiedType operator()(const FullySpecifiedType &type, Subst *subst) { return cloneType(type, subst); }
    FullySpecifiedType cloneType(const FullySpecifiedType &type, Subst *subst);

protected:
    void visit(UndefinedType *type) override;
    void visit(VoidType *type) override;
    void visit(IntegerType *type) override;
    void visit(FloatType *type) override;
    void visit(PointerToMemberType *type) override;
    void visit(PointerType *type) override;
    void visit(ReferenceType *type) override;
    void visit(ArrayType *type) override;
    void visit(NamedType *type) override;
    void visit(Function *type) override;
    void visit(Namespace *type) override;
    void visit(Template *type) override;
    void visit(Class *type) override;
    void visit(Enum *type) override;
    void visit(ForwardClassDeclaration *type) override;
    void visit(ObjCClass *type) override;
    void visit(ObjCProtocol *type) override;
    void visit(ObjCMethod *type) override;
    void visit(ObjCForwardClassDeclaration *type) override;
    void visit(ObjCForwardProtocolDeclaration *type) override;

protected:
    typedef std::pair <const FullySpecifiedType, Subst *> TypeSubstPair;
    std::map<TypeSubstPair, FullySpecifiedType> _cache;

    Clone *_clone;
    Control *_control;
    Subst *_subst;
    FullySpecifiedType _type;
};

class CPLUSPLUS_EXPORT CloneName: protected NameVisitor
{
public:
    CloneName(Clone *clone);

    const Name *operator()(const Name *name, Subst *subst) { return cloneName(name, subst); }
    const Name *cloneName(const Name *name, Subst *subst);

protected:
    void visit(const Identifier *name) override;
    void visit(const AnonymousNameId *name) override;
    void visit(const TemplateNameId *name) override;
    void visit(const DestructorNameId *name) override;
    void visit(const OperatorNameId *name) override;
    void visit(const ConversionNameId *name) override;
    void visit(const QualifiedNameId *name) override;
    void visit(const SelectorNameId *name) override;

protected:
    typedef std::pair <const Name *, Subst *> NameSubstPair;
    std::map<NameSubstPair, const Name *> _cache;

    Clone *_clone;
    Control *_control;
    Subst *_subst;
    const Name *_name;
};

class CPLUSPLUS_EXPORT CloneSymbol: protected SymbolVisitor
{
public:
    CloneSymbol(Clone *clone);

    Symbol *operator()(Symbol *symbol, Subst *subst) { return cloneSymbol(symbol, subst); }
    Symbol *cloneSymbol(Symbol *symbol, Subst *subst);

protected:
    bool visit(UsingNamespaceDirective *symbol) override;
    bool visit(UsingDeclaration *symbol) override;
    bool visit(NamespaceAlias *symbol) override;
    bool visit(Declaration *symbol) override;
    bool visit(Argument *symbol) override;
    bool visit(TypenameArgument *symbol) override;
    bool visit(BaseClass *symbol) override;
    bool visit(Enum *symbol) override;
    bool visit(Function *symbol) override;
    bool visit(Namespace *symbol) override;
    bool visit(Template *symbol) override;
    bool visit(Class *symbol) override;
    bool visit(Block *symbol) override;
    bool visit(ForwardClassDeclaration *symbol) override;

    // Qt
    bool visit(QtPropertyDeclaration *symbol) override;
    bool visit(QtEnum *symbol) override;

    // Objective-C
    bool visit(ObjCBaseClass *symbol) override;
    bool visit(ObjCBaseProtocol *symbol) override;
    bool visit(ObjCClass *symbol) override;
    bool visit(ObjCForwardClassDeclaration *symbol) override;
    bool visit(ObjCProtocol *symbol) override;
    bool visit(ObjCForwardProtocolDeclaration *symbol) override;
    bool visit(ObjCMethod *symbol) override;
    bool visit(ObjCPropertyDeclaration *symbol) override;

protected:
    typedef std::pair <Symbol *, Subst *> SymbolSubstPair;
    std::map<SymbolSubstPair, Symbol *> _cache;

    Clone *_clone;
    Control *_control;
    Subst *_subst;
    Symbol *_symbol;
};

class CPLUSPLUS_EXPORT Clone
{
    Control *_control;

public:
    Clone(Control *control);

    Control *control() const { return _control; }
    const StringLiteral *stringLiteral(const StringLiteral *literal);
    const NumericLiteral *numericLiteral(const NumericLiteral *literal);
    const Identifier *identifier(const Identifier *id);

    FullySpecifiedType type(const FullySpecifiedType &type, Subst *subst);
    const Name *name(const Name *name, Subst *subst);
    Symbol *symbol(Symbol *symbol, Subst *subst);

    Symbol *instantiate(Template *templ,
                        const FullySpecifiedType *const args, int argc,
                        Subst *subst = nullptr);

private:
    CloneType _type;
    CloneName _name;
    CloneSymbol _symbol;
};

} // end of namespace CPlusPlus
