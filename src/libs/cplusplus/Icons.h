// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/CPlusPlusForwardDeclarations.h>
#include <utils/utilsicons.h>

#include <QIcon>

namespace CPlusPlus {

class Symbol;

namespace Icons {

CPLUSPLUS_EXPORT QIcon iconForSymbol(const Symbol *symbol);

CPLUSPLUS_EXPORT QIcon keywordIcon();
CPLUSPLUS_EXPORT QIcon macroIcon();

CPLUSPLUS_EXPORT Utils::CodeModelIcon::Type iconTypeForSymbol(const Symbol *symbol);

} // namespace Icons
} // namespace CPlusPlus
