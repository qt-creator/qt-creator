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

using namespace Utils;

namespace CPlusPlus {

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

const char *Symbol::fileName() const
{ return _fileId ? _fileId->chars() : ""; }

int Symbol::fileNameLength() const
{ return _fileId ? _fileId->size() : 0; }

Utils::FilePath Symbol::filePath() const
{
    return _fileId ? Utils::FilePath::fromUtf8(_fileId->chars(), _fileId->size())
                   : Utils::FilePath();
}

const Name *Symbol::unqualifiedName() const
{
    if (! _name)
        return nullptr;

    else if (const QualifiedNameId *q = _name->asQualifiedNameId())
        return q->name();

    return _name;
}

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

Link Symbol::toLink() const
{
    int line = this->line();
    int column = this->column();

    if (column)
        --column;

    if (isGenerated())
        column = 0;

    return Link(filePath(), line, column);
}

} // CPlusPlus
