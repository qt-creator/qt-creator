/*
    Copyright (C) 2016 Volker Krause <vkrause@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to
    permit persons to whom the Software is furnished to do so, subject to
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "foldingregion.h"

using namespace KSyntaxHighlighting;

static_assert(sizeof(FoldingRegion) == 2, "FoldingRegion is size-sensitive to frequent use in KTextEditor!");

FoldingRegion::FoldingRegion() :
    m_type(None),
    m_id(0)
{
}

FoldingRegion::FoldingRegion(Type type, quint16 id) :
    m_type(type),
    m_id(id)
{
}

bool FoldingRegion::operator==(const FoldingRegion &other) const
{
    return m_id == other.m_id && m_type == other.m_type;
}

bool FoldingRegion::isValid() const
{
    return type() != None;
}

quint16 FoldingRegion::id() const
{
    return m_id;
}

FoldingRegion::Type FoldingRegion::type() const
{
    return static_cast<FoldingRegion::Type>(m_type);
}
