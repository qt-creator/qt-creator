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

#include "Symbol.h"
#include "Symbols.h"
#include "Control.h"
#include "Names.h"
#include "TranslationUnit.h"
#include "Literals.h"
#include "MemoryPool.h"
#include "SymbolVisitor.h"
#include "NameVisitor.h"
#include "Scope.h"
#include "Templates.h"

#include "cppassert.h"

#include <utils/link.h>

using namespace CPlusPlus;

class Symbol::HashCode: protected NameVisitor
{
public:
    HashCode()
        : _value(0)
    { }

    virtual ~HashCode()
    { }

    unsigned operator()(const Name *name)
    {
        unsigned previousValue = switchValue(0);
        accept(name);
        return switchValue(previousValue);
    }

protected:
    unsigned switchValue(unsigned value)
    {
        unsigned previousValue = _value;
        _value = value;
        return previousValue;
    }

    virtual void visit(const Identifier *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(const TemplateNameId *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(const DestructorNameId *name)
    { _value = name->identifier()->hashCode(); }

    virtual void visit(const OperatorNameId *name)
    { _value = unsigned(name->kind()); }

    virtual void visit(const ConversionNameId *)
    { _value = 0; } // ### TODO: implement me

    virtual void visit(const QualifiedNameId *name)
    { _value = operator()(name->name()); }

    virtual void visit(const SelectorNameId *name)
    { _value = name->identifier()->hashCode(); }

private:
    unsigned _value;
};

Symbol::Symbol(TranslationUnit *translationUnit, int sourceLocation, const Name *name)
    : _name(nullptr),
      _enclosingScope(nullptr),
      _next(nullptr),
      _fileId(nullptr),
      _sourceLocation(0),
      _hashCode(0),
      _storage(Symbol::NoStorage),
      _visibility(Symbol::Public),
      _index(0),
      _line(0),
      _column(0),
      _isGenerated(false),
      _isDeprecated(false),
      _isUnavailable(false)
{
    setSourceLocation(sourceLocation, translationUnit);
    setName(name);
}

Symbol::Symbol(Clone *clone, Subst *subst, Symbol *original)
    : _name(clone->name(original->_name, subst)),
      _enclosingScope(nullptr),
      _next(nullptr),
      _fileId(clone->control()->stringLiteral(original->fileName(), original->fileNameLength())),
      _sourceLocation(original->_sourceLocation),
      _hashCode(original->_hashCode),
      _storage(original->_storage),
      _visibility(original->_visibility),
      _index(0),
      _line(original->_line),
      _column(original->_column),
      _isGenerated(original->_isGenerated),
      _isDeprecated(original->_isDeprecated),
      _isUnavailable(original->_isUnavailable)
{
}

Symbol::~Symbol()
{ }

void Symbol::visitSymbol(SymbolVisitor *visitor)
{
    if (visitor->preVisit(this))
        visitSymbol0(visitor);
    visitor->postVisit(this);
}

void Symbol::visitSymbol(Symbol *symbol, SymbolVisitor *visitor)
{
    if (! symbol)
        return;

    symbol->visitSymbol(visitor);
}

int Symbol::sourceLocation() const
{ return _sourceLocation; }

bool Symbol::isGenerated() const
{ return _isGenerated; }

bool Symbol::isDeprecated() const
{ return _isDeprecated; }

void Symbol::setDeprecated(bool isDeprecated)
{ _isDeprecated = isDeprecated; }

bool Symbol::isUnavailable() const
{ return _isUnavailable; }

void Symbol::setUnavailable(bool isUnavailable)
{ _isUnavailable = isUnavailable; }

void Symbol::setSourceLocation(int sourceLocation, TranslationUnit *translationUnit)
{
    _sourceLocation = sourceLocation;

    if (translationUnit) {
        const Token &tk = translationUnit->tokenAt(sourceLocation);
        _isGenerated = tk.generated();
        translationUnit->getPosition(tk.utf16charsBegin(), &_line, &_column, &_fileId);
    } else {
        _isGenerated = false;
        _line = 0;
        _column = 0;
        _fileId = nullptr;
    }
}

int Symbol::line() const
{
    return _line;
}

int Symbol::column() const
{
    return _column;
}

const StringLiteral *Symbol::fileId() const
{
    return _fileId;
}

const char *Symbol::fileName() const
{ return _fileId ? _fileId->chars() : ""; }

int Symbol::fileNameLength() const
{ return _fileId ? _fileId->size() : 0; }

const Name *Symbol::unqualifiedName() const
{
    if (! _name)
        return nullptr;

    else if (const QualifiedNameId *q = _name->asQualifiedNameId())
        return q->name();

    return _name;
}

const Name *Symbol::name() const
{ return _name; }

void Symbol::setName(const Name *name)
{
    _name = name;

    if (! _name)
        _hashCode = 0;
    else {
        HashCode hh;
        _hashCode = hh(unqualifiedName());
    }
}

const Identifier *Symbol::identifier() const
{
    if (_name)
        return _name->identifier();

    return nullptr;
}

Scope *Symbol::enclosingScope() const
{ return _enclosingScope; }

void Symbol::setEnclosingScope(Scope *scope)
{
    CPP_CHECK(! _enclosingScope);
    _enclosingScope = scope;
}

void Symbol::resetEnclosingScope()
{
    _enclosingScope = nullptr;
}

Namespace *Symbol::enclosingNamespace() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Namespace *ns = s->asNamespace())
            return ns;
    }
    return nullptr;
}

Template *Symbol::enclosingTemplate() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Template *templ = s->asTemplate())
            return templ;
    }
    return nullptr;
}

Class *Symbol::enclosingClass() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Class *klass = s->asClass())
            return klass;
    }
    return nullptr;
}

Enum *Symbol::enclosingEnum() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Enum *e = s->asEnum())
            return e;
    }
    return nullptr;
}

Function *Symbol::enclosingFunction() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Function *fun = s->asFunction())
            return fun;
    }
    return nullptr;
}

Block *Symbol::enclosingBlock() const
{
    for (Scope *s = _enclosingScope; s; s = s->enclosingScope()) {
        if (Block *block = s->asBlock())
            return block;
    }
    return nullptr;
}

unsigned Symbol::index() const
{ return _index; }

Symbol *Symbol::next() const
{ return _next; }

unsigned Symbol::hashCode() const
{ return _hashCode; }

int Symbol::storage() const
{ return _storage; }

void Symbol::setStorage(int storage)
{ _storage = storage; }

int Symbol::visibility() const
{ return _visibility; }

void Symbol::setVisibility(int visibility)
{ _visibility = visibility; }

bool Symbol::isFriend() const
{ return _storage == Friend; }

bool Symbol::isRegister() const
{ return _storage == Register; }

bool Symbol::isStatic() const
{ return _storage == Static; }

bool Symbol::isExtern() const
{ return _storage == Extern; }

bool Symbol::isMutable() const
{ return _storage == Mutable; }

bool Symbol::isTypedef() const
{ return _storage == Typedef; }

bool Symbol::isPublic() const
{ return _visibility == Public; }

bool Symbol::isProtected() const
{ return _visibility == Protected; }

bool Symbol::isPrivate() const
{ return _visibility == Private; }

bool Symbol::isScope() const
{ return asScope() != nullptr; }

bool Symbol::isEnum() const
{ return asEnum()  != nullptr; }

bool Symbol::isFunction() const
{ return asFunction() != nullptr; }

bool Symbol::isNamespace() const
{ return asNamespace() != nullptr; }

bool Symbol::isTemplate() const
{ return asTemplate() != nullptr; }

bool Symbol::isClass() const
{ return asClass() != nullptr; }

bool Symbol::isForwardClassDeclaration() const
{ return asForwardClassDeclaration() != nullptr; }

bool Symbol::isQtPropertyDeclaration() const
{ return asQtPropertyDeclaration() != nullptr; }

bool Symbol::isQtEnum() const
{ return asQtEnum() != nullptr; }

bool Symbol::isBlock() const
{ return asBlock() != nullptr; }

bool Symbol::isUsingNamespaceDirective() const
{ return asUsingNamespaceDirective() != nullptr; }

bool Symbol::isUsingDeclaration() const
{ return asUsingDeclaration() != nullptr; }

bool Symbol::isDeclaration() const
{ return asDeclaration() != nullptr; }

bool Symbol::isArgument() const
{ return asArgument() != nullptr; }

bool Symbol::isTypenameArgument() const
{ return asTypenameArgument() != nullptr; }

bool Symbol::isBaseClass() const
{ return asBaseClass() != nullptr; }

bool Symbol::isObjCBaseClass() const
{ return asObjCBaseClass() != nullptr; }

bool Symbol::isObjCBaseProtocol() const
{ return asObjCBaseProtocol() != nullptr; }

bool Symbol::isObjCClass() const
{ return asObjCClass() != nullptr; }

bool Symbol::isObjCForwardClassDeclaration() const
{ return asObjCForwardClassDeclaration() != nullptr; }

bool Symbol::isObjCProtocol() const
{ return asObjCProtocol() != nullptr; }

bool Symbol::isObjCForwardProtocolDeclaration() const
{ return asObjCForwardProtocolDeclaration() != nullptr; }

bool Symbol::isObjCMethod() const
{ return asObjCMethod() != nullptr; }

bool Symbol::isObjCPropertyDeclaration() const
{ return asObjCPropertyDeclaration() != nullptr; }

void Symbol::copy(Symbol *other)
{
    _sourceLocation = other->_sourceLocation;
    _name = other->_name;
    _hashCode = other->_hashCode;
    _storage = other->_storage;
    _visibility = other->_visibility;
    _enclosingScope = other->_enclosingScope;
    _index = other->_index;
    _next = other->_next;
    _fileId = other->_fileId;
    _line = other->_line;
    _column = other->_column;

    _isGenerated = other->_isGenerated;
    _isDeprecated = other->_isDeprecated;
}

Utils::Link Symbol::toLink() const
{
    const QString filename = QString::fromUtf8(fileName(), fileNameLength());

    int line = this->line();
    int column = this->column();

    if (column)
        --column;

    if (isGenerated())
        column = 0;

    return Utils::Link(filename, line, column);
}
