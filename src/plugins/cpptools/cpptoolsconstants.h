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

const char CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS[]
    = "Builtin.EverythingWithExceptions";

const char CPP_CODE_STYLE_SETTINGS_ID[] = "A.Cpp.Code Style";
const char CPP_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Code Style");
const char CPP_FILE_SETTINGS_ID[] = "B.Cpp.File Naming";
const char CPP_FILE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "File Naming");
const char CPP_CODE_MODEL_SETTINGS_ID[] = "C.Cpp.Code Model";
const char CPP_CODE_MODEL_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "Code Model");
const char CPP_SETTINGS_CATEGORY[] = "I.C++";
const char CPP_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("CppTools", "C++");
const char SETTINGS_CATEGORY_CPP_ICON[] = ":/cpptools/images/category_cpp.png";

const char CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID[] = "ClangFixItAvailableMarker";

const char CPP_SETTINGS_ID[] = "Cpp";
const char CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppTools", "C++");

} // namespace Constants
} // namespace CppTools
