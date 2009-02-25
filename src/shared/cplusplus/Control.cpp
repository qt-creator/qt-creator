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

#include "Control.h"
#include "MemoryPool.h"
#include "Literals.h"
#include "LiteralTable.h"
#include "TranslationUnit.h"
#include "CoreTypes.h"
#include "Symbols.h"
#include "Names.h"
#include "Array.h"
#include <map> // ### replace me with LiteralTable
#include <string>

CPLUSPLUS_BEGIN_NAMESPACE

template <typename _Iterator>
static void delete_map_entries(_Iterator first, _Iterator last)
{
    for (; first != last; ++first)
        delete first->second;
}

template <typename _Map>
static void delete_map_entries(const _Map &m)
{ delete_map_entries(m.begin(), m.end()); }

template <typename _Iterator>
static void delete_array_entries(_Iterator first, _Iterator last)
{
    for (; first != last; ++first)
        delete *first;
}

template <typename _Array>
static void delete_array_entries(const _Array &a)
{ delete_array_entries(a.begin(), a.end()); }

class Control::Data
{
public:
    Data(Control *control)
        : control(control),
          translationUnit(0),
          diagnosticClient(0)
    { }

    ~Data()
    {
        // names
        delete_map_entries(nameIds);
        delete_map_entries(destructorNameIds);
        delete_map_entries(operatorNameIds);
        delete_map_entries(conversionNameIds);
        delete_map_entries(qualifiedNameIds);
        delete_map_entries(templateNameIds);

        // types
        delete_map_entries(integerTypes);
        delete_map_entries(floatTypes);
        delete_map_entries(pointerToMemberTypes);
        delete_map_entries(pointerTypes);
        delete_map_entries(referenceTypes);
        delete_map_entries(arrayTypes);
        delete_map_entries(namedTypes);

        // symbols
        delete_array_entries(declarations);
        delete_array_entries(arguments);
        delete_array_entries(functions);
        delete_array_entries(baseClasses);
        delete_array_entries(blocks);
        delete_array_entries(classes);
        delete_array_entries(namespaces);
        delete_array_entries(usingNamespaceDirectives);
        delete_array_entries(enums);
        delete_array_entries(usingDeclarations);
    }

    NameId *findOrInsertNameId(Identifier *id)
    {
        if (! id)
            return 0;
        std::map<Identifier *, NameId *>::iterator it = nameIds.lower_bound(id);
        if (it == nameIds.end() || it->first != id)
            it = nameIds.insert(it, std::make_pair(id, new NameId(id)));
        return it->second;
    }

    TemplateNameId *findOrInsertTemplateNameId(Identifier *id,
        const std::vector<FullySpecifiedType> &templateArguments)
    {
        if (! id)
            return 0;
        const TemplateNameIdKey key(id, templateArguments);
        std::map<TemplateNameIdKey, TemplateNameId *>::iterator it =
                templateNameIds.lower_bound(key);
        if (it == templateNameIds.end() || it->first != key) {
            const FullySpecifiedType *args = 0;
            if (templateArguments.size())
                args = &templateArguments[0];
            TemplateNameId *templ = new TemplateNameId(id, args,
                                                       templateArguments.size());
            it = templateNameIds.insert(it, std::make_pair(key, templ));
        }
        return it->second;
    }

    DestructorNameId *findOrInsertDestructorNameId(Identifier *id)
    {
        if (! id)
            return 0;
        std::map<Identifier *, DestructorNameId *>::iterator it = destructorNameIds.lower_bound(id);
        if (it == destructorNameIds.end() || it->first != id)
            it = destructorNameIds.insert(it, std::make_pair(id, new DestructorNameId(id)));
        return it->second;
    }

    OperatorNameId *findOrInsertOperatorNameId(int kind)
    {
        const int key(kind);
        std::map<int, OperatorNameId *>::iterator it = operatorNameIds.lower_bound(key);
        if (it == operatorNameIds.end() || it->first != key)
            it = operatorNameIds.insert(it, std::make_pair(key, new OperatorNameId(kind)));
        return it->second;
    }

    ConversionNameId *findOrInsertConversionNameId(FullySpecifiedType type)
    {
        std::map<FullySpecifiedType, ConversionNameId *>::iterator it =
                conversionNameIds.lower_bound(type);
        if (it == conversionNameIds.end() || it->first != type)
            it = conversionNameIds.insert(it, std::make_pair(type, new ConversionNameId(type)));
        return it->second;
    }

    QualifiedNameId *findOrInsertQualifiedNameId(const std::vector<Name *> &names, bool isGlobal)
    {
        const QualifiedNameIdKey key(names, isGlobal);
        std::map<QualifiedNameIdKey, QualifiedNameId *>::iterator it =
                qualifiedNameIds.lower_bound(key);
        if (it == qualifiedNameIds.end() || it->first != key) {
            QualifiedNameId *name = new QualifiedNameId(&names[0], names.size(), isGlobal);
            it = qualifiedNameIds.insert(it, std::make_pair(key, name));
        }
        return it->second;
    }

    IntegerType *findOrInsertIntegerType(int kind)
    {
        const int key = int(kind);
        std::map<int, IntegerType *>::iterator it = integerTypes.lower_bound(key);
        if (it == integerTypes.end() || it->first != key)
            it = integerTypes.insert(it, std::make_pair(key, new IntegerType(kind)));
        return it->second;
    }

    FloatType *findOrInsertFloatType(int kind)
    {
        const int key = int(kind);
        std::map<int, FloatType *>::iterator it = floatTypes.lower_bound(key);
        if (it == floatTypes.end() || it->first != key)
            it = floatTypes.insert(it, std::make_pair(key, new FloatType(kind)));
        return it->second;
    }

    PointerToMemberType *findOrInsertPointerToMemberType(Name *memberName, FullySpecifiedType elementType)
    {
        const PointerToMemberTypeKey key(memberName, elementType);
        std::map<PointerToMemberTypeKey, PointerToMemberType *>::iterator it =
                pointerToMemberTypes.lower_bound(key);
        if (it == pointerToMemberTypes.end() || it->first != key)
            it = pointerToMemberTypes.insert(it, std::make_pair(key, new PointerToMemberType(memberName, elementType)));
        return it->second;
    }

    PointerType *findOrInsertPointerType(FullySpecifiedType elementType)
    {
        std::map<FullySpecifiedType, PointerType *>::iterator it =
                pointerTypes.lower_bound(elementType);
        if (it == pointerTypes.end() || it->first != elementType)
            it = pointerTypes.insert(it, std::make_pair(elementType, new PointerType(elementType)));
        return it->second;
    }

    ReferenceType *findOrInsertReferenceType(FullySpecifiedType elementType)
    {
        std::map<FullySpecifiedType, ReferenceType *>::iterator it =
                referenceTypes.lower_bound(elementType);
        if (it == referenceTypes.end() || it->first != elementType)
            it = referenceTypes.insert(it, std::make_pair(elementType, new ReferenceType(elementType)));
        return it->second;
    }

    ArrayType *findOrInsertArrayType(FullySpecifiedType elementType, size_t size)
    {
        const ArrayKey key(elementType, size);
        std::map<ArrayKey, ArrayType *>::iterator it =
                arrayTypes.lower_bound(key);
        if (it == arrayTypes.end() || it->first != key)
            it = arrayTypes.insert(it, std::make_pair(key, new ArrayType(elementType, size)));
        return it->second;
    }

    NamedType *findOrInsertNamedType(Name *name)
    {
        std::map<Name *, NamedType *>::iterator it = namedTypes.lower_bound(name);
        if (it == namedTypes.end() || it->first != name)
            it = namedTypes.insert(it, std::make_pair(name, new NamedType(name)));
        return it->second;
    }

    Declaration *newDeclaration(unsigned sourceLocation, Name *name)
    {
        Declaration *declaration = new Declaration(translationUnit,
                                                   sourceLocation, name);
        declarations.push_back(declaration);
        return declaration;
    }

    Argument *newArgument(unsigned sourceLocation, Name *name)
    {
        Argument *argument = new Argument(translationUnit,
                                          sourceLocation, name);
        arguments.push_back(argument);
        return argument;
    }

    Function *newFunction(unsigned sourceLocation, Name *name)
    {
        Function *function = new Function(translationUnit,
                                          sourceLocation, name);
        functions.push_back(function);
        return function;
    }

    BaseClass *newBaseClass(unsigned sourceLocation, Name *name)
    {
        BaseClass *baseClass = new BaseClass(translationUnit,
                                             sourceLocation, name);
        baseClasses.push_back(baseClass);
        return baseClass;
    }

    Block *newBlock(unsigned sourceLocation)
    {
        Block *block = new Block(translationUnit, sourceLocation);
        blocks.push_back(block);
        return block;
    }

    Class *newClass(unsigned sourceLocation, Name *name)
    {
        Class *klass = new Class(translationUnit,
                                 sourceLocation, name);
        classes.push_back(klass);
        return klass;
    }

    Namespace *newNamespace(unsigned sourceLocation, Name *name)
    {
        Namespace *ns = new Namespace(translationUnit,
                                      sourceLocation, name);
        namespaces.push_back(ns);
        return ns;
    }

    UsingNamespaceDirective *newUsingNamespaceDirective(unsigned sourceLocation, Name *name)
    {
        UsingNamespaceDirective *u = new UsingNamespaceDirective(translationUnit,
                                                                 sourceLocation, name);
        usingNamespaceDirectives.push_back(u);
        return u;
    }

    Enum *newEnum(unsigned sourceLocation, Name *name)
    {
        Enum *e = new Enum(translationUnit,
                           sourceLocation, name);
        enums.push_back(e);
        return e;
    }

    UsingDeclaration *newUsingDeclaration(unsigned sourceLocation, Name *name)
    {
        UsingDeclaration *u = new UsingDeclaration(translationUnit,
                                                   sourceLocation, name);
        usingDeclarations.push_back(u);
        return u;
    }

    struct TemplateNameIdKey {
        Identifier *id;
        std::vector<FullySpecifiedType> templateArguments;

        TemplateNameIdKey(Identifier *id, const std::vector<FullySpecifiedType> &templateArguments)
            : id(id), templateArguments(templateArguments)
        { }

        bool operator == (const TemplateNameIdKey &other) const
        { return id == other.id && templateArguments == other.templateArguments; }

        bool operator != (const TemplateNameIdKey &other) const
        { return ! operator==(other); }

        bool operator < (const TemplateNameIdKey &other) const
        {
            if (id == other.id)
                return std::lexicographical_compare(templateArguments.begin(),
                                                    templateArguments.end(),
                                                    other.templateArguments.begin(),
                                                    other.templateArguments.end());
            return id < other.id;
        }
    };

    struct QualifiedNameIdKey {
        std::vector<Name *> names;
        bool isGlobal;

        QualifiedNameIdKey(const std::vector<Name *> &names, bool isGlobal) :
            names(names), isGlobal(isGlobal)
        { }

        bool operator == (const QualifiedNameIdKey &other) const
        { return isGlobal == other.isGlobal && names == other.names; }

        bool operator != (const QualifiedNameIdKey &other) const
        { return ! operator==(other); }

        bool operator < (const QualifiedNameIdKey &other) const
        {
            if (isGlobal == other.isGlobal)
                return std::lexicographical_compare(names.begin(), names.end(),
                                                    other.names.begin(), other.names.end());
            return isGlobal < other.isGlobal;
        }
    };

    struct ArrayKey {
        FullySpecifiedType type;
        size_t size;

        ArrayKey() :
            size(0)
        { }

        ArrayKey(FullySpecifiedType type, size_t size) :
            type(type), size(size)
        { }

        bool operator == (const ArrayKey &other) const
        { return type == other.type && size == other.size; }

        bool operator != (const ArrayKey &other) const
        { return ! operator==(other); }

        bool operator < (const ArrayKey &other) const
        {
            if (type == other.type)
                return size < other.size;
            return type < other.type;
        }
    };

    struct PointerToMemberTypeKey {
        Name *memberName;
        FullySpecifiedType type;

        PointerToMemberTypeKey()
            : memberName(0)
        { }

        PointerToMemberTypeKey(Name *memberName, FullySpecifiedType type)
            : memberName(memberName), type(type)
        { }

        bool operator == (const PointerToMemberTypeKey &other) const
        { return memberName == other.memberName && type == other.type; }

        bool operator != (const PointerToMemberTypeKey &other) const
        { return ! operator==(other); }

        bool operator < (const PointerToMemberTypeKey &other) const
        {
            if (memberName == other.memberName)
                return type < other.type;
            return memberName < other.memberName;
        }
    };

    Control *control;
    TranslationUnit *translationUnit;
    DiagnosticClient *diagnosticClient;
    LiteralTable<Identifier> identifiers;
    LiteralTable<StringLiteral> stringLiterals;
    LiteralTable<NumericLiteral> numericLiterals;
    LiteralTable<StringLiteral> fileNames;

    // ### replace std::map with lookup tables. ASAP!

    // names
    std::map<Identifier *, NameId *> nameIds;
    std::map<Identifier *, DestructorNameId *> destructorNameIds;
    std::map<int, OperatorNameId *> operatorNameIds;
    std::map<FullySpecifiedType, ConversionNameId *> conversionNameIds;
    std::map<TemplateNameIdKey, TemplateNameId *> templateNameIds;
    std::map<QualifiedNameIdKey, QualifiedNameId *> qualifiedNameIds;

    // types
    VoidType voidType;
    std::map<int, IntegerType *> integerTypes;
    std::map<int, FloatType *> floatTypes;
    std::map<PointerToMemberTypeKey, PointerToMemberType *> pointerToMemberTypes;
    std::map<FullySpecifiedType, PointerType *> pointerTypes;
    std::map<FullySpecifiedType, ReferenceType *> referenceTypes;
    std::map<ArrayKey, ArrayType *> arrayTypes;
    std::map<Name *, NamedType *> namedTypes;

    // symbols
    std::vector<Declaration *> declarations;
    std::vector<Argument *> arguments;
    std::vector<Function *> functions;
    std::vector<BaseClass *> baseClasses;
    std::vector<Block *> blocks;
    std::vector<Class *> classes;
    std::vector<Namespace *> namespaces;
    std::vector<UsingNamespaceDirective *> usingNamespaceDirectives;
    std::vector<Enum *> enums;
    std::vector<UsingDeclaration *> usingDeclarations;
};

Control::Control()
{ d = new Data(this); }

Control::~Control()
{ delete d; }

TranslationUnit *Control::translationUnit() const
{ return d->translationUnit; }

TranslationUnit *Control::switchTranslationUnit(TranslationUnit *unit)
{
    TranslationUnit *previousTranslationUnit = d->translationUnit;
    d->translationUnit = unit;
    return previousTranslationUnit;
}

DiagnosticClient *Control::diagnosticClient() const
{ return d->diagnosticClient; }

void Control::setDiagnosticClient(DiagnosticClient *diagnosticClient)
{ d->diagnosticClient = diagnosticClient; }

Identifier *Control::findOrInsertIdentifier(const char *chars, unsigned size)
{ return d->identifiers.findOrInsertLiteral(chars, size); }

Identifier *Control::findOrInsertIdentifier(const char *chars)
{
    unsigned length = std::char_traits<char>::length(chars);
    return findOrInsertIdentifier(chars, length);
}

Control::IdentifierIterator Control::firstIdentifier() const
{ return d->identifiers.begin(); }

Control::IdentifierIterator Control::lastIdentifier() const
{ return d->identifiers.end(); }

StringLiteral *Control::findOrInsertStringLiteral(const char *chars, unsigned size)
{ return d->stringLiterals.findOrInsertLiteral(chars, size); }

StringLiteral *Control::findOrInsertStringLiteral(const char *chars)
{
    unsigned length = std::char_traits<char>::length(chars);
    return findOrInsertStringLiteral(chars, length);
}

NumericLiteral *Control::findOrInsertNumericLiteral(const char *chars, unsigned size)
{ return d->numericLiterals.findOrInsertLiteral(chars, size); }

NumericLiteral *Control::findOrInsertNumericLiteral(const char *chars)
{
    unsigned length = std::char_traits<char>::length(chars);
    return findOrInsertNumericLiteral(chars, length);
}

unsigned Control::fileNameCount() const
{ return d->fileNames.size(); }

StringLiteral *Control::fileNameAt(unsigned index) const
{ return d->fileNames.at(index); }

StringLiteral *Control::findOrInsertFileName(const char *chars, unsigned size)
{ return d->fileNames.findOrInsertLiteral(chars, size); }

StringLiteral *Control::findOrInsertFileName(const char *chars)
{
    unsigned length = std::char_traits<char>::length(chars);
    return findOrInsertFileName(chars, length);
}

NameId *Control::nameId(Identifier *id)
{ return d->findOrInsertNameId(id); }

TemplateNameId *Control::templateNameId(Identifier *id,
       FullySpecifiedType *const args,
       unsigned argv)
{
    std::vector<FullySpecifiedType> templateArguments(args, args + argv);
    return d->findOrInsertTemplateNameId(id, templateArguments);
}

DestructorNameId *Control::destructorNameId(Identifier *id)
{ return d->findOrInsertDestructorNameId(id); }

OperatorNameId *Control::operatorNameId(int kind)
{ return d->findOrInsertOperatorNameId(kind); }

ConversionNameId *Control::conversionNameId(FullySpecifiedType type)
{ return d->findOrInsertConversionNameId(type); }

QualifiedNameId *Control::qualifiedNameId(Name *const *names,
                                             unsigned nameCount,
                                             bool isGlobal)
{
    std::vector<Name *> classOrNamespaceNames(names, names + nameCount);
    return d->findOrInsertQualifiedNameId(classOrNamespaceNames, isGlobal);
}

VoidType *Control::voidType()
{ return &d->voidType; }

IntegerType *Control::integerType(int kind)
{ return d->findOrInsertIntegerType(kind); }

FloatType *Control::floatType(int kind)
{ return d->findOrInsertFloatType(kind); }

PointerToMemberType *Control::pointerToMemberType(Name *memberName, FullySpecifiedType elementType)
{ return d->findOrInsertPointerToMemberType(memberName, elementType); }

PointerType *Control::pointerType(FullySpecifiedType elementType)
{ return d->findOrInsertPointerType(elementType); }

ReferenceType *Control::referenceType(FullySpecifiedType elementType)
{ return d->findOrInsertReferenceType(elementType); }

ArrayType *Control::arrayType(FullySpecifiedType elementType, size_t size)
{ return d->findOrInsertArrayType(elementType, size); }

NamedType *Control::namedType(Name *name)
{ return d->findOrInsertNamedType(name); }

Argument *Control::newArgument(unsigned sourceLocation, Name *name)
{ return d->newArgument(sourceLocation, name); }

Function *Control::newFunction(unsigned sourceLocation, Name *name)
{ return d->newFunction(sourceLocation, name); }

Namespace *Control::newNamespace(unsigned sourceLocation, Name *name)
{ return d->newNamespace(sourceLocation, name); }

BaseClass *Control::newBaseClass(unsigned sourceLocation, Name *name)
{ return d->newBaseClass(sourceLocation, name); }

Class *Control::newClass(unsigned sourceLocation, Name *name)
{ return d->newClass(sourceLocation, name); }

Enum *Control::newEnum(unsigned sourceLocation, Name *name)
{ return d->newEnum(sourceLocation, name); }

Block *Control::newBlock(unsigned sourceLocation)
{ return d->newBlock(sourceLocation); }

Declaration *Control::newDeclaration(unsigned sourceLocation, Name *name)
{ return d->newDeclaration(sourceLocation, name); }

UsingNamespaceDirective *Control::newUsingNamespaceDirective(unsigned sourceLocation,
                                                                Name *name)
{ return d->newUsingNamespaceDirective(sourceLocation, name); }

UsingDeclaration *Control::newUsingDeclaration(unsigned sourceLocation, Name *name)
{ return d->newUsingDeclaration(sourceLocation, name); }

CPLUSPLUS_END_NAMESPACE
