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

#include "clangsupport_global.h"

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
    HighlightingMarkContainer(uint line, uint column, uint length, HighlightingTypes types,
                              bool isIdentifier = false, bool isIncludeDirectivePath = false)
        : line_(line),
          column_(column),
          length_(length),
          types_(types),
          isIdentifier_(isIdentifier),
          isIncludeDirectivePath_(isIncludeDirectivePath)
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

    bool isInvalid() const
    {
        return line_ == 0 && column_ == 0 && length_ == 0;
    }

    bool isIdentifier() const
    {
        return isIdentifier_;
    }

    bool isIncludeDirectivePath() const
    {
        return isIncludeDirectivePath_;
    }

    friend QDataStream &operator<<(QDataStream &out, const HighlightingMarkContainer &container)
    {
        out << container.line_;
        out << container.column_;
        out << container.length_;
        out << container.types_;
        out << container.isIdentifier_;
        out << container.isIncludeDirectivePath_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, HighlightingMarkContainer &container)
    {
        in >> container.line_;
        in >> container.column_;
        in >> container.length_;
        in >> container.types_;
        in >> container.isIdentifier_;
        in >> container.isIncludeDirectivePath_;

        return in;
    }

    friend bool operator==(const HighlightingMarkContainer &first, const HighlightingMarkContainer &second)
    {
        return first.line_ == second.line_
            && first.column_ == second.column_
            && first.length_ == second.length_
            && first.types_ == second.types_
            && first.isIdentifier_ == second.isIdentifier_
            && first.isIncludeDirectivePath_ == second.isIncludeDirectivePath_;
    }

private:
    uint line_ = 0;
    uint column_ = 0;
    uint length_ = 0;
    HighlightingTypes types_;
    bool isIdentifier_ = false;
    bool isIncludeDirectivePath_ = false;
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

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const HighlightingMarkContainer &container);
CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, HighlightingType highlightingType);
CLANGSUPPORT_EXPORT std::ostream &operator<<(std::ostream &os, HighlightingTypes types);
std::ostream &operator<<(std::ostream &os, const HighlightingMarkContainer &container);

} // namespace ClangBackEnd
