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

namespace LanguageClient {
namespace Constants {

const char LANGUAGECLIENT_SETTINGS_CATEGORY[] = "ZY.LanguageClient";
const char LANGUAGECLIENT_SETTINGS_PAGE[] = "LanguageClient.General";
const char LANGUAGECLIENT_STDIO_SETTINGS_ID[] = "LanguageClient::StdIOSettingsID";
const char LANGUAGECLIENT_SETTINGS_TR[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Language Client");
const char LANGUAGECLIENT_DOCUMENT_FILTER_ID[] = "Current Document Symbols";
const char LANGUAGECLIENT_DOCUMENT_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Symbols in Current Document");
const char LANGUAGECLIENT_DOCUMENT_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates symbols in the current document, based on a language server.");
const char LANGUAGECLIENT_WORKSPACE_FILTER_ID[] = "Workspace Symbols";
const char LANGUAGECLIENT_WORKSPACE_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Symbols in Workspace");
const char LANGUAGECLIENT_WORKSPACE_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Locates symbols in the language server workspace.");
const char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_ID[] = "Workspace Classes and Structs";
const char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Classes and Structs in Workspace");
const char LANGUAGECLIENT_WORKSPACE_CLASS_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates classes and structs in the language server workspace.");
const char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_ID[] = "Workspace Functions and Methods";
const char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("QtC::LanguageClient", "Functions and Methods in Workspace");
const char LANGUAGECLIENT_WORKSPACE_METHOD_FILTER_DESCRIPTION[]
    = QT_TRANSLATE_NOOP("QtC::LanguageClient",
                        "Locates functions and methods in the language server workspace.");

const char CALL_HIERARCHY_FACTORY_ID[] = "LanguageClient.CallHierarchy";

} // namespace Constants
} // namespace LanguageClient
