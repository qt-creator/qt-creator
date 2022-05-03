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

namespace CppEditor {
namespace Constants {

const char M_CONTEXT[] = "CppEditor.ContextMenu";
const char G_CONTEXT_FIRST[] = "CppEditor.GFirst";
const char CPPEDITOR_ID[] = "CppEditor.C++Editor";
const char CPPEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "C++ Editor");
const char SWITCH_DECLARATION_DEFINITION[] = "CppEditor.SwitchDeclarationDefinition";
const char OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT[] = "CppEditor.OpenDeclarationDefinitionInNextSplit";
const char OPEN_PREPROCESSOR_DIALOG[] = "CppEditor.OpenPreprocessorDialog";
const char MULTIPLE_PARSE_CONTEXTS_AVAILABLE[] = "CppEditor.MultipleParseContextsAvailable";
const char M_REFACTORING_MENU_INSERTION_POINT[] = "CppEditor.RefactorGroup";
const char UPDATE_CODEMODEL[] = "CppEditor.UpdateCodeModel";
const char INSPECT_CPP_CODEMODEL[] = "CppEditor.InspectCppCodeModel";

const char TYPE_HIERARCHY_ID[] = "CppEditor.TypeHierarchy";
const char OPEN_TYPE_HIERARCHY[] = "CppEditor.OpenTypeHierarchy";

const char INCLUDE_HIERARCHY_ID[] = "CppEditor.IncludeHierarchy";
const char OPEN_INCLUDE_HIERARCHY[] = "CppEditor.OpenIncludeHierarchy";

const char CPP_SNIPPETS_GROUP_ID[] = "C++";

const char EXTRA_PREPROCESSOR_DIRECTIVES[] = "CppEditor.ExtraPreprocessorDirectives-";
const char PREFERRED_PARSE_CONTEXT[] = "CppEditor.PreferredParseContext-";

const char QUICK_FIX_PROJECT_PANEL_ID[] = "CppEditor.QuickFix";
const char QUICK_FIX_SETTINGS_ID[] = "CppEditor.QuickFix";
const char QUICK_FIX_SETTINGS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "Quick Fixes");
const char QUICK_FIX_SETTING_GETTER_OUTSIDE_CLASS_FROM[] = "GettersOutsideClassFrom";
const char QUICK_FIX_SETTING_GETTER_IN_CPP_FILE_FROM[] = "GettersInCppFileFrom";
const char QUICK_FIX_SETTING_SETTER_OUTSIDE_CLASS_FROM[] = "SettersOutsideClassFrom";
const char QUICK_FIX_SETTING_SETTER_IN_CPP_FILE_FROM[] = "SettersInCppFileFrom";
const char QUICK_FIX_SETTING_GETTER_ATTRIBUTES[] = "GetterAttributes";
const char QUICK_FIX_SETTING_GETTER_NAME_TEMPLATE[] = "GetterNameTemplate";
const char QUICK_FIX_SETTING_SETTER_NAME_TEMPLATE[] = "SetterNameTemplate";
const char QUICK_FIX_SETTING_SIGNAL_NAME_TEMPLATE[] = "SignalNameTemplate";
const char QUICK_FIX_SETTING_RESET_NAME_TEMPLATE[] = "ResetNameTemplate";
const char QUICK_FIX_SETTING_SIGNAL_WITH_NEW_VALUE[] = "SignalWithNewValue";
const char QUICK_FIX_SETTING_SETTER_AS_SLOT[] = "SetterAsSlot";
const char QUICK_FIX_SETTING_SETTER_PARAMETER_NAME[] = "SetterParameterName";
const char QUICK_FIX_SETTING_CPP_FILE_NAMESPACE_HANDLING[] = "CppFileNamespaceHandling";
const char QUICK_FIX_SETTING_MEMBER_VARIABEL_NAME_TEMPLATE[] = "MemberVariableNameTemplate";
const char QUICK_FIX_SETTING_VALUE_TYPES[] = "ValueTypes";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATES[] = "CustomTemplate";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_TYPES[] = "Types";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_COMPARISON[] = "Comparison";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_TYPE[] = "ReturnType";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_EXPRESSION[] = "ReturnExpression";
const char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_ASSIGNMENT[] = "Assignment";

const char M_TOOLS_CPP[]              = "CppTools.Tools.Menu";
const char SWITCH_HEADER_SOURCE[]     = "CppTools.SwitchHeaderSource";
const char OPEN_HEADER_SOURCE_IN_NEXT_SPLIT[] = "CppTools.OpenHeaderSourceInNextSplit";
const char TASK_INDEX[]               = "CppTools.Task.Index";
const char TASK_SEARCH[]              = "CppTools.Task.Search";
const char C_SOURCE_MIMETYPE[] = "text/x-csrc";
const char CUDA_SOURCE_MIMETYPE[] = "text/vnd.nvidia.cuda.csrc";
const char C_HEADER_MIMETYPE[] = "text/x-chdr";
const char CPP_SOURCE_MIMETYPE[] = "text/x-c++src";
const char OBJECTIVE_C_SOURCE_MIMETYPE[] = "text/x-objcsrc";
const char OBJECTIVE_CPP_SOURCE_MIMETYPE[] = "text/x-objc++src";
const char CPP_HEADER_MIMETYPE[] = "text/x-c++hdr";
const char QDOC_MIMETYPE[] = "text/x-qdoc";
const char MOC_MIMETYPE[] = "text/x-moc";
const char AMBIGUOUS_HEADER_MIMETYPE[] = "application/vnd.qtc.ambiguousheader"; // not a real MIME type

// QSettings keys for use by the "New Class" wizards.
const char CPPEDITOR_SETTINGSGROUP[] = "CppTools";
const char LOWERCASE_CPPFILES_KEY[] = "LowerCaseFiles";
const bool LOWERCASE_CPPFILES_DEFAULT = true;
const char CPPEDITOR_SORT_EDITOR_DOCUMENT_OUTLINE[] = "SortedMethodOverview";
const char CPPEDITOR_MODEL_MANAGER_PCH_USAGE[] = "PCHUsage";
const char CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS[]
    = "InterpretAmbiguousHeadersAsCHeaders";
const char CPPEDITOR_SKIP_INDEXING_BIG_FILES[] = "SkipIndexingBigFiles";
const char CPPEDITOR_INDEXER_FILE_SIZE_LIMIT[] = "IndexerFileSizeLimit";

const char CPP_CLANG_DIAG_CONFIG_QUESTIONABLE[] = "Builtin.Questionable";
const char CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM[] = "Builtin.BuildSystem";

const char CPP_CODE_STYLE_SETTINGS_ID[] = "A.Cpp.Code Style";
const char CPP_CODE_STYLE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "Code Style");
const char CPP_FILE_SETTINGS_ID[] = "B.Cpp.File Naming";
const char CPP_FILE_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "File Naming");
const char CPP_CODE_MODEL_SETTINGS_ID[] = "C.Cpp.Code Model";
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_ID[] = "C.Cpp.Diagnostic Config";
const char CPP_DIAGNOSTIC_CONFIG_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "Diagnostic Configurations");
const char CPP_CLANGD_SETTINGS_ID[] = "K.Cpp.Clangd";
const char CPP_SETTINGS_CATEGORY[] = "I.C++";

const char CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID[] = "ClangFixItAvailableMarker";
const char CPP_FUNCTION_DECL_DEF_LINK_MARKER_ID[] = "FunctionDeclDefLinkMarker";

const char CPP_SETTINGS_ID[] = "Cpp";
const char CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++");

const char CURRENT_DOCUMENT_FILTER_ID[] = "Methods in current Document";
const char CURRENT_DOCUMENT_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++ Symbols in Current Document");

const char CLASSES_FILTER_ID[] = "Classes";
const char CLASSES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++ Classes");

const char FUNCTIONS_FILTER_ID[] = "Methods";
const char FUNCTIONS_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++ Functions");

const char INCLUDES_FILTER_ID[] = "All Included C/C++ Files";
const char INCLUDES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "All Included C/C++ Files");

const char LOCATOR_FILTER_ID[] = "Classes and Methods";
const char LOCATOR_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++ Classes, Enums, Functions and Type Aliases");

const char SYMBOLS_FIND_FILTER_ID[] = "Symbols";
const char SYMBOLS_FIND_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("CppEditor", "C++ Symbols");

constexpr const char CLANG_STATIC_ANALYZER_DOCUMENTATION_URL[]
    = "https://clang-analyzer.llvm.org/available_checks.html";

} // namespace Constants
} // namespace CppEditor
