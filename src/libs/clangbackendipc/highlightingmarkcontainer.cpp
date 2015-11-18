/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "highlightingmarkcontainer.h"

#include <QDataStream>
#include <QDebug>

#include <ostream>

namespace ClangBackEnd {

HighlightingMarkContainer::HighlightingMarkContainer(uint line,
                                                     uint column,
                                                     uint length,
                                                     HighlightingType type)
    : line_(line),
      column_(column),
      length_(length),
      type_(type)
{
}

uint HighlightingMarkContainer::line() const
{
    return line_;
}

uint HighlightingMarkContainer::column() const
{
    return column_;
}

uint HighlightingMarkContainer::length() const
{
    return length_;
}

HighlightingType HighlightingMarkContainer::type() const
{
    return type_;
}

quint32 &HighlightingMarkContainer::typeAsInt()
{
    return reinterpret_cast<quint32&>(type_);
}

QDataStream &operator<<(QDataStream &out, const HighlightingMarkContainer &container)
{
    out << container.line_;
    out << container.column_;
    out << container.length_;
    out << quint32(container.type_);

    return out;
}

QDataStream &operator>>(QDataStream &in, HighlightingMarkContainer &container)
{
    in >> container.line_;
    in >> container.column_;
    in >> container.length_;
    in >> container.typeAsInt();

    return in;
}

bool operator==(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second)
{
    return first.line_ == second.line_
        && first.column_ == second.column_
        && first.length_ == second.length_
        && first.type_ == second.type_;
}

bool operator<(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second)
{
    if (first.line() == second.line()) {
        if (first.column() == second.column()) {
            if (first.length() == second.length())
                return first.type() < second.type();
            return first.length() < second.length();
        }

        return first.column() < second.column();
    }

    return first.line() < second.line();
}

#define RETURN_TEXT_FOR_CASE(enumValue) case HighlightingType::enumValue: return #enumValue
static const char *highlightingTypeToCStringLiteral(HighlightingType type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(Invalid);
        RETURN_TEXT_FOR_CASE(Comment);
        RETURN_TEXT_FOR_CASE(Keyword);
        RETURN_TEXT_FOR_CASE(StringLiteral);
        RETURN_TEXT_FOR_CASE(NumberLiteral);
        RETURN_TEXT_FOR_CASE(Function);
        RETURN_TEXT_FOR_CASE(VirtualFunction);
        RETURN_TEXT_FOR_CASE(Type);
        RETURN_TEXT_FOR_CASE(LocalVariable);
        RETURN_TEXT_FOR_CASE(GlobalVariable);
        RETURN_TEXT_FOR_CASE(Field);
        RETURN_TEXT_FOR_CASE(Enumeration);
        RETURN_TEXT_FOR_CASE(Operator);
        RETURN_TEXT_FOR_CASE(Preprocessor);
        RETURN_TEXT_FOR_CASE(Label);
        RETURN_TEXT_FOR_CASE(OutputArgument);
        RETURN_TEXT_FOR_CASE(PreprocessorDefinition);
        RETURN_TEXT_FOR_CASE(PreprocessorExpansion);
        default: return "UnhandledHighlightingType";
    }
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, const HighlightingMarkContainer &container)
{
    debug.nospace() << "HighlightingMarkContainer("
                    << container.line() << ", "
                    << container.column() << ", "
                    << container.length() << ", "
                    << highlightingTypeToCStringLiteral(container.type()) << ", "
                    << ")";

    return debug;
}

void PrintTo(HighlightingType highlightingType, std::ostream *os)
{
    *os << highlightingTypeToCStringLiteral(highlightingType);
}

void PrintTo(const HighlightingMarkContainer& container, ::std::ostream *os)
{
    *os << "HighlightingMarkContainer("
        << container.line() << ", "
        << container.column() << ", "
        << container.length() << ", ";
    PrintTo(container.type(), os);
    *os << ")";
}

} // namespace ClangBackEnd
