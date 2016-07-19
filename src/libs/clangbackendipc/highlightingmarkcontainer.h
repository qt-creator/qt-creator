/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "clangbackendipc_global.h"

#include <QDataStream>

#include <iosfwd>

namespace ClangBackEnd {

inline QDataStream &operator<<(QDataStream &out, HighlightingType highlightingType);
inline QDataStream &operator<<(QDataStream &out, HighlightingTypes highlightingTypes);
inline QDataStream &operator>>(QDataStream &in, HighlightingType &highlightingType);
inline QDataStream &operator>>(QDataStream &in, HighlightingTypes &highlightingTypes);
inline bool operator==(const MixinHighlightingTypes &first, const MixinHighlightingTypes &second);
inline bool operator==(const HighlightingTypes &first, const HighlightingTypes &second);

class HighlightingMarkContainer
{
public:
    HighlightingMarkContainer() = default;
    HighlightingMarkContainer(uint line, uint column, uint length, HighlightingTypes types)
        : line_(line),
          column_(column),
          length_(length),
          types_(types)
    {
    }

    HighlightingMarkContainer(uint line, uint column, uint length, HighlightingType type)
        : line_(line),
          column_(column),
          length_(length)
    {
        types_.mainHighlightingType = type;
    }

    uint line() const
    {
        return line_;
    }

    uint column() const
    {
        return column_;
    }

    uint length() const
    {
        return length_;
    }

    HighlightingTypes types() const
    {
        return types_;
    }

    friend QDataStream &operator<<(QDataStream &out, const HighlightingMarkContainer &container)
    {
        out << container.line_;
        out << container.column_;
        out << container.length_;
        out << container.types_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, HighlightingMarkContainer &container)
    {
        in >> container.line_;
        in >> container.column_;
        in >> container.length_;
        in >> container.types_;

        return in;
    }

    friend bool operator==(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second)
    {
        return first.line_ == second.line_
            && first.column_ == second.column_
            && first.length_ == second.length_
            && first.types_ == second.types_;
    }

private:
    uint line_;
    uint column_;
    uint length_;
    HighlightingTypes types_;
};

inline QDataStream &operator<<(QDataStream &out, HighlightingType highlightingType)
{
    out << static_cast<const quint8>(highlightingType);

    return out;
}

inline QDataStream &operator<<(QDataStream &out, HighlightingTypes highlightingTypes)
{
    out << highlightingTypes.mainHighlightingType;

    out << highlightingTypes.mixinHighlightingTypes.size();

    for (HighlightingType type : highlightingTypes.mixinHighlightingTypes)
        out << type;

    return out;
}


inline QDataStream &operator>>(QDataStream &in, HighlightingType &highlightingType)
{
    quint8 highlightingTypeInt;

    in >> highlightingTypeInt;

    highlightingType = static_cast<HighlightingType>(highlightingTypeInt);

    return in;
}

inline QDataStream &operator>>(QDataStream &in, HighlightingTypes &highlightingTypes)
{
    in >> highlightingTypes.mainHighlightingType ;

    quint8 size;
    in >> size;

    for (int counter = 0; counter < size; ++counter) {
        HighlightingType type;
        in >> type;
        highlightingTypes.mixinHighlightingTypes.push_back(type);
    }

    return in;
}

inline bool operator==(const MixinHighlightingTypes &first, const MixinHighlightingTypes &second)
{
    return first.size() == second.size()
        && std::equal(first.begin(), first.end(), second.begin());
}

inline bool operator==(const HighlightingTypes &first, const HighlightingTypes &second)
{
    return first.mainHighlightingType == second.mainHighlightingType
        && first.mixinHighlightingTypes == second.mixinHighlightingTypes;
}

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const HighlightingMarkContainer &container);
CMBIPC_EXPORT void PrintTo(HighlightingType highlightingType, ::std::ostream *os);
CMBIPC_EXPORT void PrintTo(const HighlightingTypes &types, ::std::ostream *os);
void PrintTo(const HighlightingMarkContainer &container, ::std::ostream *os);

} // namespace ClangBackEnd
