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

#ifndef CPLUSPLUS_CONTROL_H
#define CPLUSPLUS_CONTROL_H

#include "CPlusPlusForwardDeclarations.h"
#include <cstddef>


namespace CPlusPlus {

class CPLUSPLUS_EXPORT Control
{
public:
    Control();
    ~Control();

    TranslationUnit *translationUnit() const;
    TranslationUnit *switchTranslationUnit(TranslationUnit *unit);

    DiagnosticClient *diagnosticClient() const;
    void setDiagnosticClient(DiagnosticClient *diagnosticClient);

    /// Returns the canonical name id.
    NameId *nameId(Identifier *id);

    /// Returns the canonical template name id.
    TemplateNameId *templateNameId(Identifier *id,
                                   FullySpecifiedType *const args = 0,
                                   unsigned argc = 0);

    /// Returns the canonical destructor name id.
    DestructorNameId *destructorNameId(Identifier *id);

    /// Returns the canonical operator name id.
    OperatorNameId *operatorNameId(int operatorId);

    /// Returns the canonical conversion name id.
    ConversionNameId *conversionNameId(const FullySpecifiedType &type);

    /// Returns the canonical qualified name id.
    QualifiedNameId *qualifiedNameId(Name *const *names,
                                     unsigned nameCount,
                                     bool isGlobal = false);

    SelectorNameId *selectorNameId(Name *const *names,
                                   unsigned nameCount,
                                   bool hasArguments);

    /// Returns a Type object of type VoidType.
    VoidType *voidType();

    /// Returns a Type object of type IntegerType.
    IntegerType *integerType(int integerId);

    /// Returns a Type object of type FloatType.
    FloatType *floatType(int floatId);

    /// Returns a Type object of type PointertoMemberType.
    PointerToMemberType *pointerToMemberType(Name *memberName,
                                             FullySpecifiedType elementType);

    /// Returns a Type object of type PointerType.
    PointerType *pointerType(const FullySpecifiedType &elementType);

    /// Returns a Type object of type ReferenceType.
    ReferenceType *referenceType(const FullySpecifiedType &elementType);

    /// Retruns a Type object of type ArrayType.
    ArrayType *arrayType(const FullySpecifiedType &elementType, size_t size = 0);

    /// Returns a Type object of type NamedType.
    NamedType *namedType(Name *name);

    /// Creates a new Declaration symbol.
    Declaration *newDeclaration(unsigned sourceLocation, Name *name);

    /// Creates a new Argument symbol.
    Argument *newArgument(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Function symbol.
    Function *newFunction(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Namespace symbol.
    Namespace *newNamespace(unsigned sourceLocation, Name *name = 0);

    /// Creates a new BaseClass symbol.
    BaseClass *newBaseClass(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Class symbol.
    Class *newClass(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Enum symbol.
    Enum *newEnum(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Block symbol.
    Block *newBlock(unsigned sourceLocation);

    /// Creates a new UsingNamespaceDirective symbol.
    UsingNamespaceDirective *newUsingNamespaceDirective(unsigned sourceLocation, Name *name = 0);

    /// Creates a new UsingDeclaration symbol.
    UsingDeclaration *newUsingDeclaration(unsigned sourceLocation, Name *name = 0);

    /// Creates a new ForwardClassDeclaration symbol.
    ForwardClassDeclaration *newForwardClassDeclaration(unsigned sourceLocation, Name *name = 0);

    ObjCBaseClass *newObjCBaseClass(unsigned sourceLocation, Name *name);
    ObjCBaseProtocol *newObjCBaseProtocol(unsigned sourceLocation, Name *name);

    /// Creates a new Objective-C class symbol.
    ObjCClass *newObjCClass(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Objective-C class forward declaration symbol.
    ObjCForwardClassDeclaration *newObjCForwardClassDeclaration(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Objective-C protocol symbol.
    ObjCProtocol *newObjCProtocol(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Objective-C protocol forward declaration symbol.
    ObjCForwardProtocolDeclaration *newObjCForwardProtocolDeclaration(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Objective-C method symbol.
    ObjCMethod *newObjCMethod(unsigned sourceLocation, Name *name = 0);

    /// Creates a new Objective-C @property declaration symbol.
    ObjCPropertyDeclaration *newObjCPropertyDeclaration(unsigned sourceLocation, Name *name);

    // Objective-C specific context keywords.
    Identifier *objcGetterId() const;
    Identifier *objcSetterId() const;
    Identifier *objcReadwriteId() const;
    Identifier *objcReadonlyId() const;
    Identifier *objcAssignId() const;
    Identifier *objcRetainId() const;
    Identifier *objcCopyId() const;
    Identifier *objcNonatomicId() const;

    Identifier *findIdentifier(const char *chars, unsigned size) const;

    Identifier *findOrInsertIdentifier(const char *chars, unsigned size);
    Identifier *findOrInsertIdentifier(const char *chars);

    typedef const Identifier *const *IdentifierIterator;
    typedef const StringLiteral *const *StringLiteralIterator;
    typedef const NumericLiteral *const *NumericLiteralIterator;

    IdentifierIterator firstIdentifier() const;
    IdentifierIterator lastIdentifier() const;

    StringLiteralIterator firstStringLiteral() const;
    StringLiteralIterator lastStringLiteral() const;

    NumericLiteralIterator firstNumericLiteral() const;
    NumericLiteralIterator lastNumericLiteral() const;

    StringLiteral *findOrInsertStringLiteral(const char *chars, unsigned size);
    StringLiteral *findOrInsertStringLiteral(const char *chars);

    NumericLiteral *findOrInsertNumericLiteral(const char *chars, unsigned size);
    NumericLiteral *findOrInsertNumericLiteral(const char *chars);

private:
    class Data;
    friend class Data;
    Data *d;
};

} // end of namespace CPlusPlus


#endif // CPLUSPLUS_CONTROL_H
