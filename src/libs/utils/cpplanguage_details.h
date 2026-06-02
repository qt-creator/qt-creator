// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    CXX20,
    CXX23,
    LatestCxx = CXX23,
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

enum class WarningFlag {
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
Q_DECLARE_FLAGS(WarningFlags, WarningFlag)

enum class QtMajorVersion { Unknown = -1, None, Qt4, Qt5, Qt6 };

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::LanguageExtensions)
Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::WarningFlags)
