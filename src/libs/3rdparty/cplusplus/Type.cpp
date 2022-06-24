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

#include "Matcher.h"
#include "Type.h"
#include "TypeVisitor.h"
#include "CoreTypes.h"

using namespace CPlusPlus;

Type::~Type() = default;

bool Type::isUndefinedType() const
{ return this == &UndefinedType::instance; }

void Type::accept(TypeVisitor *visitor)
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void Type::accept(Type *type, TypeVisitor *visitor)
{
    if (! type)
        return;

    type->accept(visitor);
}

bool Type::match(const Type *other, Matcher *matcher) const
{
    return Matcher::match(this, other, matcher);
}
