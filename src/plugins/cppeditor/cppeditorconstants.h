// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace CppEditor::Constants {

inline constexpr char M_CONTEXT[] = "CppEditor.ContextMenu";
inline constexpr char G_SYMBOL[] = "CppEditor.GSymbol";
inline constexpr char G_SELECTION[] = "CppEditor.GSelection";
inline constexpr char G_FILE[] = "CppEditor.GFile";
inline constexpr char G_GLOBAL[] = "CppEditor.GGlobal";
inline constexpr char CPPEDITOR_ID[] = "CppEditor.C++Editor";
inline constexpr char SWITCH_DECLARATION_DEFINITION[] = "CppEditor.SwitchDeclarationDefinition";
inline constexpr char OPEN_DECLARATION_DEFINITION_IN_NEXT_SPLIT[] = "CppEditor.OpenDeclarationDefinitionInNextSplit";
inline constexpr char OPEN_PREPROCESSOR_DIALOG[] = "CppEditor.OpenPreprocessorDialog";
inline constexpr char MULTIPLE_PARSE_CONTEXTS_AVAILABLE[] = "CppEditor.MultipleParseContextsAvailable";
inline constexpr char M_REFACTORING_MENU_INSERTION_POINT[] = "CppEditor.RefactorGroup";
inline constexpr char UPDATE_CODEMODEL[] = "CppEditor.UpdateCodeModel";
inline constexpr char INSPECT_CPP_CODEMODEL[] = "CppEditor.InspectCppCodeModel";

inline constexpr char INCLUDE_HIERARCHY_ID[] = "CppEditor.IncludeHierarchy";
inline constexpr char OPEN_INCLUDE_HIERARCHY[] = "CppEditor.OpenIncludeHierarchy";

inline constexpr char CPP_SNIPPETS_GROUP_ID[] = "C++";

inline constexpr char EXTRA_PREPROCESSOR_DIRECTIVES[] = "CppEditor.ExtraPreprocessorDirectives-";
inline constexpr char PREFERRED_PARSE_CONTEXT[] = "CppEditor.PreferredParseContext-";

inline constexpr char QUICK_FIX_PROJECT_PANEL_ID[] = "CppEditor.QuickFix";
inline constexpr char QUICK_FIX_SETTINGS_ID[] = "CppEditor.QuickFix";
inline constexpr char QUICK_FIX_SETTINGS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::CppEditor", "Quick Fixes");
inline constexpr char QUICK_FIX_SETTING_GETTER_OUTSIDE_CLASS_FROM[] = "GettersOutsideClassFrom";
inline constexpr char QUICK_FIX_SETTING_GETTER_IN_CPP_FILE_FROM[] = "GettersInCppFileFrom";
inline constexpr char QUICK_FIX_SETTING_SETTER_OUTSIDE_CLASS_FROM[] = "SettersOutsideClassFrom";
inline constexpr char QUICK_FIX_SETTING_SETTER_IN_CPP_FILE_FROM[] = "SettersInCppFileFrom";
inline constexpr char QUICK_FIX_SETTING_GETTER_ATTRIBUTES[] = "GetterAttributes";
inline constexpr char QUICK_FIX_SETTING_GETTER_NAME_TEMPLATE[] = "GetterNameTemplateV2";
inline constexpr char QUICK_FIX_SETTING_SETTER_NAME_TEMPLATE[] = "SetterNameTemplateV2";
inline constexpr char QUICK_FIX_SETTING_SIGNAL_NAME_TEMPLATE[] = "SignalNameTemplateV2";
inline constexpr char QUICK_FIX_SETTING_RESET_NAME_TEMPLATE[] = "ResetNameTemplateV2";
inline constexpr char QUICK_FIX_SETTING_SIGNAL_WITH_NEW_VALUE[] = "SignalWithNewValue";
inline constexpr char QUICK_FIX_SETTING_SETTER_AS_SLOT[] = "SetterAsSlot";
inline constexpr char QUICK_FIX_SETTING_SETTER_PARAMETER_NAME[] = "SetterParameterNameV2";
inline constexpr char QUICK_FIX_SETTING_CPP_FILE_NAMESPACE_HANDLING[] = "CppFileNamespaceHandling";
inline constexpr char QUICK_FIX_SETTING_USE_AUTO[] = "UseAutoInAssignToVar";
inline constexpr char QUICK_FIX_SETTING_MEMBER_VARIABLE_NAME_TEMPLATE[] = "MemberVariableNameTemplateV2";
inline constexpr char QUICK_FIX_SETTING_REVERSE_MEMBER_VARIABLE_NAME_TEMPLATE[] = "ReverseMemberVariableNameTemplate";
inline constexpr char QUICK_FIX_SETTING_VALUE_TYPES[] = "ValueTypes";
inline constexpr char QUICK_FIX_SETTING_RETURN_BY_CONST_REF[] = "ReturnNonValueTypesByConstRef";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATES[] = "CustomTemplate";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_TYPES[] = "Types";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_COMPARISON[] = "Comparison";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_TYPE[] = "ReturnType";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_RETURN_EXPRESSION[] = "ReturnExpression";
inline constexpr char QUICK_FIX_SETTING_CUSTOM_TEMPLATE_ASSIGNMENT[] = "Assignment";

inline constexpr char M_TOOLS_CPP[]              = "CppTools.Tools.Menu";
inline constexpr char SWITCH_HEADER_SOURCE[]     = "CppTools.SwitchHeaderSource";
inline constexpr char OPEN_HEADER_SOURCE_IN_NEXT_SPLIT[] = "CppTools.OpenHeaderSourceInNextSplit";
inline constexpr char SHOW_PREPROCESSED_FILE[]     = "CppTools.ShowPreprocessedFile";
inline constexpr char SHOW_PREPROCESSED_FILE_SPLIT[]     = "CppTools.ShowPreprocessedFileSplit";
inline constexpr char TASK_INDEX[]               = "CppTools.Task.Index";
inline constexpr char TASK_SEARCH[]              = "CppTools.Task.Search";

// QSettings keys for use by the "New Class" wizards.
inline constexpr char CPPEDITOR_SETTINGSGROUP[] = "CppTools";
inline constexpr char LOWERCASE_CPPFILES_KEY[] = "LowerCaseFiles";
const bool LOWERCASE_CPPFILES_DEFAULT = true;
inline constexpr char CPPEDITOR_SORT_EDITOR_DOCUMENT_OUTLINE[] = "SortedMethodOverview";
inline constexpr char CPPEDITOR_MODEL_MANAGER_PCH_USAGE[] = "PCHUsage";
inline constexpr char CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS[]
    = "InterpretAmbiguousHeadersAsCHeaders";
inline constexpr char CPPEDITOR_USE_BUILTIN_PREPROCESSOR[] = "UseBuiltinPreprocessor";
inline constexpr char CPPEDITOR_SKIP_INDEXING_BIG_FILES[] = "SkipIndexingBigFiles";
inline constexpr char CPPEDITOR_INDEXER_FILE_SIZE_LIMIT[] = "IndexerFileSizeLimit";
inline constexpr char CPPEDITOR_IGNORE_FILES[] = "IgnoreFiles";
inline constexpr char CPPEDITOR_IGNORE_PATTERN[] = "IgnorePattern";

inline constexpr char CPP_CLANG_DIAG_CONFIG_QUESTIONABLE[] = "Builtin.Questionable";
inline constexpr char CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM[] = "Builtin.BuildSystem";

inline constexpr char CPP_CODE_STYLE_SETTINGS_ID[] = "A.Cpp.Code Style";
inline constexpr char CPP_FILE_SETTINGS_ID[] = "B.Cpp.File Naming";
inline constexpr char CPP_CODE_MODEL_SETTINGS_ID[] = "C.Cpp.Code Model";
inline constexpr char CPP_DIAGNOSTIC_CONFIG_SETTINGS_ID[] = "C.Cpp.Diagnostic Config";
inline constexpr char CPP_CLANGD_SETTINGS_ID[] = "K.Cpp.Clangd";
inline constexpr char CPP_SETTINGS_CATEGORY[] = "I.C++";

inline constexpr char CPP_CLANG_FIXIT_AVAILABLE_MARKER_ID[] = "ClangFixItAvailableMarker";
inline constexpr char CPP_FUNCTION_DECL_DEF_LINK_MARKER_ID[] = "FunctionDeclDefLinkMarker";

inline constexpr char CPP_SETTINGS_ID[] = "Cpp";
inline constexpr char CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("QtC::CppEditor", "C++");

inline constexpr char CURRENT_DOCUMENT_FILTER_ID[] = "Methods in current Document";

inline constexpr char CLASSES_FILTER_ID[] = "Classes";

inline constexpr char FUNCTIONS_FILTER_ID[] = "Methods";

inline constexpr char INCLUDES_FILTER_ID[] = "All Included C/C++ Files";
inline constexpr char INCLUDES_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::CppEditor",
                                                              "All Included C/C++ Files");

inline constexpr char LOCATOR_FILTER_ID[] = "Classes and Methods";

inline constexpr char SYMBOLS_FIND_FILTER_ID[] = "Symbols";
inline constexpr char SYMBOLS_FIND_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::CppEditor", "C++ Symbols");

constexpr const char CLANG_STATIC_ANALYZER_DOCUMENTATION_URL[]
    = "https://clang-analyzer.llvm.org/available_checks.html";

inline constexpr char CLANGD_TOOL_ID[] = "DockerDeviceClangDExecutable";

} // namespace CppEditor::Constants
