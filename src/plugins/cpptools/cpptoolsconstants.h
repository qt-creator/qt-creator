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

#include <QtGlobal>

namespace CppTools {
namespace Constants {

const char M_TOOLS_CPP[]              = "CppTools.Tools.Menu";
const char SWITCH_HEADER_SOURCE[]     = "CppTools.SwitchHeaderSource";
const char OPEN_HEADER_SOURCE_IN_NEXT_SPLIT[] = "CppTools.OpenHeaderSourceInNextSplit";
const char TASK_INDEX[]               = "CppTools.Task.Index";
const char TASK_SEARCH[]              = "CppTools.Task.Search";
const char C_SOURCE_MIMETYPE[] = "text/x-csrc";
const char C_HEADER_MIMETYPE[] = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char OBJECTIVE_C_SOURCE_MIMETYPE[] = "text/x-objcsrc";
const char OBJECTIVE_CPP_SOURCE_MIMETYPE[] = "text/x-objc++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";
const char QDOC_MIMETYPE[] = "text/x-qdoc";
const char MOC_MIMETYPE[] = "text/x-moc";
const char AMBIGUOUS_HEADER_MIMETYPE[] = "application/vnd.qtc.ambiguousheader"; // not a real MIME type

// QSettings keys for use by the "New Class" wizards.
const char CPPTOOLS_SETTINGSGROUP[] = "CppTools";
const char LOWERCASE_CPPFILES_KEY[] = "LowerCaseFiles";
enum { lowerCaseFilesDefault = 1 };
const char CPPTOOLS_SORT_EDITOR_DOCUMENT_OUTLINE[] = "SortedMethodOverview";
const char CPPTOOLS_SHOW_INFO_BAR_FOR_HEADER_ERRORS[] = "ShowInfoBarForHeaderErrors";
const char CPPTOOLS_SHOW_INFO_BAR_FOR_FOR_NO_PROJECT[] = "ShowInfoBarForNoProject";
const char CPPTOOLS_MODEL_MANAGER_PCH_USAGE[] = "PCHUsage";
const char CPPTOOLS_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS[]
    = "InterpretAmbiguousHeadersAsCHeaders";
const char CPPTOOLS_SKIP_INDEXING_BIG_FILES[] = "SkipIndexingBigFiles";
const char CPPTOOLS_INDEXER_FILE_SIZE_LIMIT[] = "IndexerFileSizeLimit";

const char CPP_CLANG_DIAG_CONFIG_QUESTIONABLE[] = "Builtin.Questionable";

const char CPP_CODE_STYLE_SETTINGS_ID[] = "A.Cpp.Code Style";
const char CPP_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Code Style");
const char CPP_FILE_SETTINGS_ID[] = "B.Cpp.File Naming";
const char CPP_FILE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "File Naming");
const char CPP_CODE_MODEL_SETTINGS_ID[] = "C.Cpp.Code Model";
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_ID[] = "C.Cpp.Diagnostic Config";
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Diagnostic Configurations");
const char CPP_SETTINGS_CATEGORY[] = "I.C++";

const char CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID[] = "ClangFixItAvailableMarker";
const char CPP_FUNCTION_DECL_DEF_LINK_MARKER_ID[] = "FunctionDeclDefLinkMarker";

const char CPP_SETTINGS_ID[] = "Cpp";
const char CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++");

const char CURRENT_DOCUMENT_FILTER_ID[] = "Methods in current Document";
const char CURRENT_DOCUMENT_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Symbols in Current Document");

const char CLASSES_FILTER_ID[] = "Classes";
const char CLASSES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Classes");

const char FUNCTIONS_FILTER_ID[] = "Methods";
const char FUNCTIONS_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Functions");

const char INCLUDES_FILTER_ID[] = "All Included C/C++ Files";
const char INCLUDES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "All Included C/C++ Files");

const char LOCATOR_FILTER_ID[] = "Classes and Methods";
const char LOCATOR_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Classes, Enums and Functions");

const char SYMBOLS_FIND_FILTER_ID[] = "Symbols";
const char SYMBOLS_FIND_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++ Symbols");

// CLANG-UPGRADE-CHECK: Checks/update URLs.
//
// Upgrade the version in the URL. Note that we cannot use the macro
// CLANG_VERSION here because it might denote a version that was not yet
// released (e.g. 6.0.1, but only 6.0.0 was released).
constexpr const char TIDY_DOCUMENTATION_URL_TEMPLATE[]
    = "https://releases.llvm.org/8.0.1/tools/clang/tools/extra/docs/clang-tidy/checks/%1.html";

constexpr const char CLANG_STATIC_ANALYZER_DOCUMENTATION_URL[]
    = "https://clang-analyzer.llvm.org/available_checks.html";

// CLANG-UPGRADE-CHECK: Checks/update URLs.
//
// Once it gets dedicated documentation pages for released versions,
// use them instead of pointing to master, as checks might vanish.
constexpr const char CLAZY_DOCUMENTATION_URL_TEMPLATE[]
    = "https://github.com/KDE/clazy/blob/master/docs/checks/README-%1.md";

} // namespace Constants
} // namespace CppTools
