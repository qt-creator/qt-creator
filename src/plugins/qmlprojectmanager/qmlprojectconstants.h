// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QmlProjectManager::Constants {

inline constexpr char customFileSelectorsData[] = "CustomFileSelectorsData";
inline constexpr char supportedLanguagesData[] = "SupportedLanguagesData";
inline constexpr char primaryLanguageData[] = "PrimaryLanguageData";
inline constexpr char customForceFreeTypeData[] = "CustomForceFreeType";
inline constexpr char customQtForMCUs[] = "CustomQtForMCUs";
inline constexpr char customQt6Project[] = "CustomQt6Project";
inline constexpr char customDefaultFontFamilyMCU[] = "CustomDefaultFontFamilyMCU";

inline constexpr char mainFilePath[] = "MainFilePath";
inline constexpr char canonicalProjectDir[] ="CanonicalProjectDir";

inline constexpr char ALWAYS_OPEN_UI_MODE[] = "J.QtQuick/QmlJSEditor.openUiQmlMode";
inline constexpr char QML_RESOURCE_PATH[] = "qmldesigner/propertyEditorQmlSources/imports";
inline constexpr char LANDING_PAGE_PATH[] = "qmldesigner/landingpage";

inline constexpr char QML_PROJECT_ID[] = "QmlProjectManager.QmlProject";
inline constexpr char QML_RUNCONFIG_ID[] = "QmlProjectManager.QmlRunConfiguration.Qml";
inline constexpr char QML_VIEWER_KEY[] = "QmlProjectManager.QmlRunConfiguration.QDeclarativeViewer";
inline constexpr char QML_VIEWER_ARGUMENTS_KEY[] = "QmlProjectManager.QmlRunConfiguration.QDeclarativeViewerArguments";
inline constexpr char QML_VIEWER_TARGET_DISPLAY_NAME[] = "QML Viewer";
inline constexpr char QML_MAINSCRIPT_KEY[] = "QmlProjectManager.QmlRunConfiguration.MainScript";
inline constexpr char USE_MULTILANGUAGE_KEY[] = "QmlProjectManager.QmlRunConfiguration.UseMultiLanguage";
inline constexpr char LAST_USED_LANGUAGE[] = "QmlProjectManager.QmlRunConfiguration.LastUsedLanguage";
inline constexpr char USER_ENVIRONMENT_CHANGES_KEY[] = "QmlProjectManager.QmlRunConfiguration.UserEnvironmentChanges";

inline constexpr char EXPORT_MENU[] = "QmlDesigner.ExportMenu";
inline constexpr char G_EXPORT_GENERATE[] = "QmlDesigner.Group.GenerateProject";
inline constexpr char G_EXPORT_CONVERT[] = "QmlDesigner.Group.ConvertProject";

inline constexpr char fakeProjectName[] = "fake85673.qmlproject";

inline constexpr const char *QDS_FONT_FILES_FILTERS[] = {
    "*.afm", "*.bdf", "*.ccc", "*.cff", "*.fmp",  "*.fnt", "*.otc", "*.otf",  "*.pcf",   "*.pfa",
    "*.pfb", "*.pfm", "*.pfr", "*.ttc", "*.ttcf", "*.tte", "*.ttf", "*.woff", "*.woff2",
};
constexpr char FALLBACK_MCU_FONT_FAMILY[] = "DejaVu Sans";
// These constants should be kept in sync with their counterparts in qmlbase.h
constexpr char QMLPUPPET_ENV_MCU_FONTS_DIR[] = "QMLPUPPET_MCU_FONTS_DIR";
constexpr char QMLPUPPET_ENV_DEFAULT_FONT_FAMILY[] = "QMLPUPPET_DEFAULT_FONT_FAMILY";
constexpr char QMLPUPPET_ENV_PROJECT_ROOT[] = "QMLPUPPET_PROJECT_ROOT";

} // QmlProjectManager::Constants
