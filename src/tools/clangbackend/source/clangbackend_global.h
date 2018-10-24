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

#include <clang-c/Index.h>

namespace ClangBackEnd {

enum class PreferredTranslationUnit
{
    RecentlyParsed,
    PreviouslyParsed,
    LastUninitialized,
};

// CLANG-UPGRADE-CHECK: Remove IS_PRETTY_DECL_SUPPORTED once we require clang >= 7.0
#if defined(CINDEX_VERSION_HAS_PRETTYDECL_BACKPORTED) || CINDEX_VERSION_MINOR >= 46
#  define IS_PRETTY_DECL_SUPPORTED
#endif

// CLANG-UPGRADE-CHECK: Remove IS_INVALIDDECL_SUPPORTED once we require clang >= 7.0
#if defined(CINDEX_VERSION_HAS_ISINVALIDECL_BACKPORTED) || CINDEX_VERSION_MINOR >= 46
#  define IS_INVALIDDECL_SUPPORTED
#endif

// CLANG-UPGRADE-CHECK: Remove IS_LIMITSKIPFUNCTIONBODIESTOPREAMBLE_SUPPORTED once we require clang >= 7.0
#if defined(CINDEX_VERSION_HAS_LIMITSKIPFUNCTIONBODIESTOPREAMBLE_BACKPORTED) || CINDEX_VERSION_MINOR >= 46
#  define IS_LIMITSKIPFUNCTIONBODIESTOPREAMBLE_SUPPORTED
#endif

// CLANG-UPGRADE-CHECK: Remove IS_SKIPWARNINGSFROMINCLUDEDFILES_SUPPORTED
#if defined(CINDEX_VERSION_HAS_SKIPWARNINGSFROMINCLUDEDFILES_BACKPORTED)
#  define IS_SKIPWARNINGSFROMINCLUDEDFILES_SUPPORTED
#endif

// CLANG-UPGRADE-CHECK: Remove IS_COMPLETION_FIXITS_BACKPORTED once we require clang >= 7.0
#if defined(CINDEX_VERSION_HAS_COMPLETION_FIXITS_BACKPORTED) || CINDEX_VERSION_MINOR >= 49
#  define IS_COMPLETION_FIXITS_BACKPORTED
#endif

} // namespace ClangBackEnd
