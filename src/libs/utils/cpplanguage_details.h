/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QFlags>

namespace Utils {

enum class Language : unsigned char { None, C, Cxx };

enum class LanguageVersion : unsigned char {
    None,
    C89,
    C99,
    C11,
    C18,
    LatestC = C18,
    CXX98 = 32,
    CXX03,
    CXX11,
    CXX14,
    CXX17,
    CXX2a,
    LatestCxx = CXX2a,
};

enum class LanguageExtension : unsigned char {
    None = 0,

    Gnu = 1 << 0,
    Microsoft = 1 << 1,
    Borland = 1 << 2,
    OpenMP = 1 << 3,
    ObjectiveC = 1 << 4,

    All = Gnu | Microsoft | Borland | OpenMP | ObjectiveC
};

enum class WarningFlags {
    // General settings
    NoWarnings = 0,
    AsErrors = 1 << 0,
    Default = 1 << 1,
    All = 1 << 2,
    Extra = 1 << 3,
    Pedantic = 1 << 4,

    // Any language
    UnusedLocals = 1 << 7,
    UnusedParams = 1 << 8,
    UnusedFunctions = 1 << 9,
    UnusedResult = 1 << 10,
    UnusedValue = 1 << 11,
    Documentation = 1 << 12,
    UninitializedVars = 1 << 13,
    HiddenLocals = 1 << 14,
    UnknownPragma = 1 << 15,
    Deprecated = 1 << 16,
    SignedComparison = 1 << 17,
    IgnoredQualifiers = 1 << 18,

    // C++
    OverloadedVirtual = 1 << 24,
    EffectiveCxx = 1 << 25,
    NonVirtualDestructor = 1 << 26
};

Q_DECLARE_FLAGS(LanguageExtensions, LanguageExtension)

enum class QtVersion { Unknown = -1, None, Qt4, Qt5 };

} // namespace Utils

constexpr Utils::LanguageExtension operator|(Utils::LanguageExtension first,
                                             Utils::LanguageExtension second)
{
    return Utils::LanguageExtension(static_cast<unsigned char>(first)
                                    | static_cast<unsigned char>(second));
}

constexpr bool operator&&(Utils::LanguageExtension first, Utils::LanguageExtension second)
{
    return static_cast<unsigned char>(first) & static_cast<unsigned char>(second);
}

inline Utils::WarningFlags operator|(Utils::WarningFlags first, Utils::WarningFlags second)
{
    return Utils::WarningFlags(int(first) | int(second));
}

inline Utils::WarningFlags operator&(Utils::WarningFlags first, Utils::WarningFlags second)
{
    return Utils::WarningFlags(int(first) & int(second));
}

inline void operator|=(Utils::WarningFlags &first, Utils::WarningFlags second)
{
    first = first | second;
}

inline void operator&=(Utils::WarningFlags &first, Utils::WarningFlags second)
{
    first = first & second;
}

inline Utils::WarningFlags operator~(Utils::WarningFlags flags)
{
    return Utils::WarningFlags(~int(flags));
}
