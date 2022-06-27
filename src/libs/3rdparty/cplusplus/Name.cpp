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

#include "Literals.h"
#include "Matcher.h"
#include "Name.h"
#include "Names.h"
#include "NameVisitor.h"

#include <cstring>
#include <string_view>

using namespace CPlusPlus;

Name::Name()
{ }

Name::~Name()
{ }

void Name::accept(NameVisitor *visitor) const
{
    if (visitor->preVisit(this))
        accept0(visitor);
    visitor->postVisit(this);
}

void Name::accept(const Name *name, NameVisitor *visitor)
{
    if (! name)
        return;
    name->accept(visitor);
}

bool Name::match(const Name *other, Matcher *matcher) const
{
    return Matcher::match(this, other, matcher);
}

bool Name::Equals::operator()(const Name *name, const Name *other) const
{
    if (name == other)
        return true;
    if (name == nullptr || other == nullptr)
        return false;

    const Identifier *id = name->identifier();
    const Identifier *otherId = other->identifier();

    if (id == otherId)
        return true;
    if (id == nullptr || otherId == nullptr)
        return false;

    return std::strcmp(id->chars(), otherId->chars()) == 0;
}

size_t Name::Hash::operator()(const Name *name) const
{
    if (name == nullptr)
        return 0;

    const Identifier *id = name->identifier();

    if (id == nullptr)
        return 0;

    return std::hash<std::string_view>()(std::string_view(id->chars()));
}
