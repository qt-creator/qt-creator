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
#include "ASTfwd.h"
#include "Names.h"

namespace CPlusPlus {

class CPLUSPLUS_EXPORT TopLevelDeclarationProcessor
{
public:
    virtual ~TopLevelDeclarationProcessor() {}
    virtual bool processDeclaration(DeclarationAST *ast) = 0;
};

class CPLUSPLUS_EXPORT Control
{
public:
    Control();
    ~Control();

    TranslationUnit *translationUnit() const;
    TranslationUnit *switchTranslationUnit(TranslationUnit *unit);

    TopLevelDeclarationProcessor *topLevelDeclarationProcessor() const;
    void setTopLevelDeclarationProcessor(TopLevelDeclarationProcessor *processor);

    DiagnosticClient *diagnosticClient() const;
    void setDiagnosticClient(DiagnosticClient *diagnosticClient);

    /// Returns the canonical anonymous name id
    const AnonymousNameId *anonymousNameId(unsigned classTokenIndex);

    /// Returns the canonical template name id.
    const TemplateNameId *templateNameId(const Identifier *id,
                                         bool isSpecialization,
                                         const TemplateArgument *const args = nullptr,
                                         int argc = 0);

    /// Returns the canonical destructor name id.
    const DestructorNameId *destructorNameId(const Name *name);

    /// Returns the canonical operator name id.
    const OperatorNameId *operatorNameId(OperatorNameId::Kind operatorId);

    /// Returns the canonical conversion name id.
    const ConversionNameId *conversionNameId(const FullySpecifiedType &type);

    /// Returns the canonical qualified name id.
    const QualifiedNameId *qualifiedNameId(const Name *base, const Name *name);

    const SelectorNameId *selectorNameId(const Name *const *names,
                                         int nameCount,
                                         bool hasArguments);

    /// Returns a Type object of type VoidType.
    VoidType *voidType();

    /// Returns a Type object of type IntegerType.
    IntegerType *integerType(int integerId);

    /// Returns a Type object of type FloatType.
    FloatType *floatType(int floatId);

    /// Returns a Type object of type PointertoMemberType.
    PointerToMemberType *pointerToMemberType(const Name *memberName,
                                             const FullySpecifiedType &elementType);

    /// Returns a Type object of type PointerType.
    PointerType *pointerType(const FullySpecifiedType &elementType);

    /// Returns a Type object of type ReferenceType.
    ReferenceType *referenceType(const FullySpecifiedType &elementType, bool rvalueRef);

    /// Retruns a Type object of type ArrayType.
    ArrayType *arrayType(const FullySpecifiedType &elementType, int size = 0);

    /// Returns a Type object of type NamedType.
    NamedType *namedType(const Name *name);

    /// Creates a new Declaration symbol.
    Declaration *newDeclaration(int sourceLocation, const Name *name);

    /// Creates a new EnumeratorDeclaration symbol.
    EnumeratorDeclaration *newEnumeratorDeclaration(int sourceLocation, const Name *name);

    /// Creates a new Argument symbol.
    Argument *newArgument(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Argument symbol.
    TypenameArgument *newTypenameArgument(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Function symbol.
    Function *newFunction(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Namespace symbol.
    Namespace *newNamespace(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Template symbol.
    Template *newTemplate(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Namespace symbol.
    NamespaceAlias *newNamespaceAlias(int sourceLocation, const Name *name = nullptr);

    /// Creates a new BaseClass symbol.
    BaseClass *newBaseClass(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Class symbol.
    Class *newClass(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Enum symbol.
    Enum *newEnum(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Block symbol.
    Block *newBlock(int sourceLocation);

    /// Creates a new UsingNamespaceDirective symbol.
    UsingNamespaceDirective *newUsingNamespaceDirective(int sourceLocation, const Name *name = nullptr);

    /// Creates a new UsingDeclaration symbol.
    UsingDeclaration *newUsingDeclaration(int sourceLocation, const Name *name = nullptr);

    /// Creates a new ForwardClassDeclaration symbol.
    ForwardClassDeclaration *newForwardClassDeclaration(int sourceLocation, const Name *name = nullptr);

    /// Creates a new QtPropertyDeclaration symbol.
    QtPropertyDeclaration *newQtPropertyDeclaration(int sourceLocation, const Name *name = nullptr);

    /// Creates a new QtEnum symbol.
    QtEnum *newQtEnum(int sourceLocation, const Name *name = nullptr);

    ObjCBaseClass *newObjCBaseClass(int sourceLocation, const Name *name);
    ObjCBaseProtocol *newObjCBaseProtocol(int sourceLocation, const Name *name);

    /// Creates a new Objective-C class symbol.
    ObjCClass *newObjCClass(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Objective-C class forward declaration symbol.
    ObjCForwardClassDeclaration *newObjCForwardClassDeclaration(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Objective-C protocol symbol.
    ObjCProtocol *newObjCProtocol(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Objective-C protocol forward declaration symbol.
    ObjCForwardProtocolDeclaration *newObjCForwardProtocolDeclaration(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Objective-C method symbol.
    ObjCMethod *newObjCMethod(int sourceLocation, const Name *name = nullptr);

    /// Creates a new Objective-C @property declaration symbol.
    ObjCPropertyDeclaration *newObjCPropertyDeclaration(int sourceLocation, const Name *name);

    const Identifier *deprecatedId() const;
    const Identifier *unavailableId() const;
    // Objective-C specific context keywords.
    const Identifier *objcGetterId() const;
    const Identifier *objcSetterId() const;
    const Identifier *objcReadwriteId() const;
    const Identifier *objcReadonlyId() const;
    const Identifier *objcAssignId() const;
    const Identifier *objcRetainId() const;
    const Identifier *objcCopyId() const;
    const Identifier *objcNonatomicId() const;
    // C++11 context keywords
    const Identifier *cpp11Override() const;
    const Identifier *cpp11Final() const;

    const OperatorNameId *findOperatorNameId(OperatorNameId::Kind operatorId) const;

    const Identifier *findIdentifier(const char *chars, int size) const;
    const Identifier *identifier(const char *chars, int size);
    const Identifier *identifier(const char *chars);

    typedef const Identifier *const *IdentifierIterator;
    typedef const StringLiteral *const *StringLiteralIterator;
    typedef const NumericLiteral *const *NumericLiteralIterator;

    IdentifierIterator firstIdentifier() const;
    IdentifierIterator lastIdentifier() const;

    StringLiteralIterator firstStringLiteral() const;
    StringLiteralIterator lastStringLiteral() const;

    NumericLiteralIterator firstNumericLiteral() const;
    NumericLiteralIterator lastNumericLiteral() const;

    const StringLiteral *stringLiteral(const char *chars, int size);
    const StringLiteral *stringLiteral(const char *chars);

    const NumericLiteral *numericLiteral(const char *chars, int size);
    const NumericLiteral *numericLiteral(const char *chars);

    Symbol **firstSymbol() const;
    Symbol **lastSymbol() const;
    int symbolCount() const;

    bool hasSymbol(Symbol *symbol) const;
    void addSymbol(Symbol *symbol);

private:
    class Data;
    friend class Data;
    Data *d;
};

} // namespace CPlusPlus
