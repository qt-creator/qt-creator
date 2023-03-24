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

namespace Utils {
class FilePath;
class Link;
} // Utils

namespace CPlusPlus {

class CPLUSPLUS_EXPORT Symbol
{
    Symbol(const Symbol &other);
    void operator =(const Symbol &other);

public:
    /// Storage class specifier
    enum Storage {
        NoStorage = 0,
        Friend,
        Auto,
        Register,
        Static,
        Extern,
        Mutable,
        Typedef
    };

    /// Access specifier.
    enum Visibility {
        Public,
        Protected,
        Private,
        Package
    };

public:
    /// Constructs a Symbol with the given source location, name and translation unit.
    Symbol(TranslationUnit *translationUnit, int sourceLocation, const Name *name);
    Symbol(Clone *clone, Subst *subst, Symbol *original);

    /// Destroy this Symbol.
    virtual ~Symbol();

    /// Returns this Symbol's source location.
    int sourceLocation() const { return _sourceLocation; }

    /// Returns this Symbol's line number. The line number is 1-based.
    int line() const { return _line; }

    /// Returns this Symbol's column number. The column number is 1-based.
    int column() const { return _column; }

    /// Returns this Symbol's file name.
    const StringLiteral *fileId() const { return _fileId; }

    /// Returns this Symbol's file name.
    const char *fileName() const;

    /// Returns this Symbol's file name length.
    int fileNameLength() const;

    Utils::FilePath filePath() const;

    /// Returns this Symbol's name.
    const Name *name() const { return _name; }

    /// Sets this Symbol's name.
    void setName(const Name *name); // ### dangerous

    /// Returns this Symbol's (optional) identifier
    const Identifier *identifier() const;

    /// Returns this Symbol's storage class specifier.
    int storage() const { return _storage; }

    /// Sets this Symbol's storage class specifier.
    void setStorage(int storage) { _storage = storage; }

    /// Returns this Symbol's visibility.
    int visibility() const { return _visibility; }

    /// Sets this Symbol's visibility.
    void setVisibility(int visibility) { _visibility = visibility; }

    /// Returns the next chained Symbol.
    Symbol *next() const { return _next; }

    /// Returns true if this Symbol has friend storage specifier.
    bool isFriend() const { return _storage == Friend; }

    /// Returns true if this Symbol has register storage specifier.
    bool isRegister() const { return _storage == Register; }

    /// Returns true if this Symbol has static storage specifier.
    bool isStatic() const { return _storage == Static; }

    /// Returns true if this Symbol has extern storage specifier.
    bool isExtern() const { return _storage == Extern; }

    /// Returns true if this Symbol has mutable storage specifier.
    bool isMutable() const { return _storage == Mutable; }

    /// Returns true if this Symbol has typedef storage specifier.
    bool isTypedef() const { return _storage == Typedef; }

    /// Returns true if this Symbol's visibility is public.
    bool isPublic() const { return _visibility == Public; }

    /// Returns true if this Symbol's visibility is protected.
    bool isProtected() const { return _visibility == Protected; }

    /// Returns true if this Symbol's visibility is private.
    bool isPrivate() const { return _visibility == Private; }

    Utils::Link toLink() const;

    virtual const Scope *asScope() const { return nullptr; }
    virtual const Enum *asEnum() const { return nullptr; }
    virtual const Function *asFunction() const { return nullptr; }
    virtual const Namespace *asNamespace() const { return nullptr; }
    virtual const Template *asTemplate() const { return nullptr; }
    virtual const NamespaceAlias *asNamespaceAlias() const { return nullptr; }
    virtual const Class *asClass() const { return nullptr; }
    virtual const Block *asBlock() const { return nullptr; }
    virtual const UsingNamespaceDirective *asUsingNamespaceDirective() const { return nullptr; }
    virtual const UsingDeclaration *asUsingDeclaration() const { return nullptr; }
    virtual const Declaration *asDeclaration() const { return nullptr; }
    virtual const Argument *asArgument() const { return nullptr; }
    virtual const TypenameArgument *asTypenameArgument() const { return nullptr; }
    virtual const BaseClass *asBaseClass() const { return nullptr; }
    virtual const ForwardClassDeclaration *asForwardClassDeclaration() const { return nullptr; }
    virtual const QtPropertyDeclaration *asQtPropertyDeclaration() const { return nullptr; }
    virtual const QtEnum *asQtEnum() const { return nullptr; }
    virtual const ObjCBaseClass *asObjCBaseClass() const { return nullptr; }
    virtual const ObjCBaseProtocol *asObjCBaseProtocol() const { return nullptr; }
    virtual const ObjCClass *asObjCClass() const { return nullptr; }
    virtual const ObjCForwardClassDeclaration *asObjCForwardClassDeclaration() const { return nullptr; }
    virtual const ObjCProtocol *asObjCProtocol() const { return nullptr; }
    virtual const ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration() const { return nullptr; }
    virtual const ObjCMethod *asObjCMethod() const { return nullptr; }
    virtual const ObjCPropertyDeclaration *asObjCPropertyDeclaration() const { return nullptr; }

    /// Returns this Symbol as a Scope.
    virtual Scope *asScope() { return nullptr; }

    /// Returns this Symbol as an Enum.
    virtual Enum *asEnum() { return nullptr; }

    /// Returns this Symbol as an Function.
    virtual Function *asFunction() { return nullptr; }

    /// Returns this Symbol as a Namespace.
    virtual Namespace *asNamespace() { return nullptr; }

    /// Returns this Symbol as a Template.
    virtual Template *asTemplate() { return nullptr; }

    virtual NamespaceAlias *asNamespaceAlias() { return nullptr; }

    /// Returns this Symbol as a Class.
    virtual Class *asClass() { return nullptr; }

    /// Returns this Symbol as a Block.
    virtual Block *asBlock() { return nullptr; }

    /// Returns this Symbol as a UsingNamespaceDirective.
    virtual UsingNamespaceDirective *asUsingNamespaceDirective() { return nullptr; }

    /// Returns this Symbol as a UsingDeclaration.
    virtual UsingDeclaration *asUsingDeclaration() { return nullptr; }

    /// Returns this Symbol as a Declaration.
    virtual Declaration *asDeclaration() { return nullptr; }

    /// Returns this Symbol as an Argument.
    virtual Argument *asArgument() { return nullptr; }

    /// Returns this Symbol as a Typename argument.
    virtual TypenameArgument *asTypenameArgument() { return nullptr; }

    /// Returns this Symbol as a BaseClass.
    virtual BaseClass *asBaseClass() { return nullptr; }

    /// Returns this Symbol as a ForwardClassDeclaration.
    virtual ForwardClassDeclaration *asForwardClassDeclaration() { return nullptr; }

    /// Returns this Symbol as a QtPropertyDeclaration.
    virtual QtPropertyDeclaration *asQtPropertyDeclaration() { return nullptr; }

    /// Returns this Symbol as a QtEnum.
    virtual QtEnum *asQtEnum() { return nullptr; }

    virtual ObjCBaseClass *asObjCBaseClass() { return nullptr; }
    virtual ObjCBaseProtocol *asObjCBaseProtocol() { return nullptr; }

    /// Returns this Symbol as an Objective-C Class declaration.
    virtual ObjCClass *asObjCClass() { return nullptr; }

    /// Returns this Symbol as an Objective-C Class forward declaration.
    virtual ObjCForwardClassDeclaration *asObjCForwardClassDeclaration() { return nullptr; }

    /// Returns this Symbol as an Objective-C Protocol declaration.
    virtual ObjCProtocol *asObjCProtocol() { return nullptr; }

    /// Returns this Symbol as an Objective-C Protocol forward declaration.
    virtual ObjCForwardProtocolDeclaration *asObjCForwardProtocolDeclaration() { return nullptr; }

    /// Returns this Symbol as an Objective-C method declaration.
    virtual ObjCMethod *asObjCMethod() { return nullptr; }

    /// Returns this Symbol as an Objective-C @property declaration.
    virtual ObjCPropertyDeclaration *asObjCPropertyDeclaration() { return nullptr; }

    /// Returns this Symbol's type.
    virtual FullySpecifiedType type() const = 0;

    /// Returns this Symbol's hash value.
    unsigned hashCode() const { return _hashCode; }

    /// Returns this Symbol's index.
    unsigned index() const { return _index; }

    const Name *unqualifiedName() const;

    bool isGenerated() const { return _isGenerated; }

    bool isDeprecated() const { return _isDeprecated; }
    void setDeprecated(bool isDeprecated) { _isDeprecated = isDeprecated; }

    bool isUnavailable() const { return _isUnavailable; }
    void setUnavailable(bool isUnavailable) { _isUnavailable = isUnavailable; }

    /// Returns this Symbol's eclosing scope.
    Scope *enclosingScope() const { return _enclosingScope; }

    /// Returns the eclosing namespace scope.
    Namespace *enclosingNamespace() const;

    /// Returns the eclosing template scope.
    Template *enclosingTemplate() const;

    /// Returns the enclosing class scope.
    Class *enclosingClass() const;

    /// Returns the enclosing enum scope.
    Enum *enclosingEnum() const;

    /// Returns the enclosing prototype scope.
    Function *enclosingFunction() const;

    /// Returns the enclosing Block scope.
    Block *enclosingBlock() const;

    void setEnclosingScope(Scope *enclosingScope); // ### make me private
    void resetEnclosingScope(); // ### make me private
    void setSourceLocation(int sourceLocation, TranslationUnit *translationUnit); // ### make me private

    void visitSymbol(SymbolVisitor *visitor);
    static void visitSymbol(Symbol *symbol, SymbolVisitor *visitor);

    virtual void copy(Symbol *other);

protected:
    virtual void visitSymbol0(SymbolVisitor *visitor) = 0;

private:
    const Name *_name;
    Scope *_enclosingScope;
    Symbol *_next;
    const StringLiteral *_fileId;
    int _sourceLocation;
    unsigned _hashCode;
    int _storage;
    int _visibility;
    int _index;
    int _line;
    int _column;

    bool _isGenerated: 1;
    bool _isDeprecated: 1;
    bool _isUnavailable: 1;

    class HashCode;

    friend class SymbolTable;
};

} // namespace CPlusPlus
