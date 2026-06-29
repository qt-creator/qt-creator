// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(LANGUAGECLIENT_LIBRARY)
#  define LANGUAGECLIENT_EXPORT Q_DECL_EXPORT
#elif defined(LANGUAGECLIENT_STATIC_LIBRARY)
#  define LANGUAGECLIENT_EXPORT
#else
#  define LANGUAGECLIENT_EXPORT Q_DECL_IMPORT
#endif

namespace LanguageClient::Constants {

inline constexpr char LANGUAGECLIENT_SETTINGS_CATEGORY[] = "ZY.LanguageClient";
inline constexpr char LANGUAGECLIENT_SETTINGS_PAGE[] = "LanguageClient.General";
inline constexpr char LANGUAGECLIENT_SETTINGS_PANEL[] = "LanguageClient.General";
inline constexpr char LANGUAGECLIENT_STDIO_SETTINGS_ID[] = "LanguageClient::StdIOSettingsID";
inline constexpr char LANGUAGECLIENT_SETTINGS_TR[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Language Client");
inline constexpr char LANGUAGECLIENT_DOCUMENT_FILTER_ID[] = "Current Document Symbols";
inline constexpr char LANGUAGECLIENT_DOCUMENT_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Symbols in Current Document");
inline constexpr char LANGUAGECLIENT_DOCUMENT_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates symbols in the current document, based on a language server.");
inline constexpr char LANGUAGECLIENT_WORKSPACE_FILTER_ID[] = "Workspace Symbols";
inline constexpr char LANGUAGECLIENT_WORKSPACE_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Symbols in Workspace");
inline constexpr char LANGUAGECLIENT_WORKSPACE_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Locates symbols in the language server workspace.");
inline constexpr char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_ID[] = "Workspace Classes and Structs";
inline constexpr char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Classes and Structs in Workspace");
inline constexpr char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates classes and structs in the language server workspace.");
inline constexpr char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_ID[] = "Workspace Functions and Methods";
inline constexpr char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Functions and Methods in Workspace");
inline constexpr char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates functions and methods in the language server workspace.");

inline constexpr char CALL_HIERARCHY_FACTORY_ID[] = "LanguageClient.CallHierarchy";
inline constexpr char TASK_CATEGORY_DIAGNOSTICS[] = "LanguageClient.DiagnosticTask";

} // namespace LanguageClient::Constants
