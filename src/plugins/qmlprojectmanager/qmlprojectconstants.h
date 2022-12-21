// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljstools/qmljstoolsconstants.h>

namespace QmlProjectManager {
namespace Constants {

const char * const QMLPROJECT_MIMETYPE = QmlJSTools::Constants::QMLPROJECT_MIMETYPE;
const char customFileSelectorsData[] = "CustomFileSelectorsData";
const char supportedLanguagesData[] = "SupportedLanguagesData";
const char primaryLanguageData[] = "PrimaryLanguageData";
const char customForceFreeTypeData[] = "CustomForceFreeType";
const char customQtForMCUs[] = "CustomQtForMCUs";
const char customQt6Project[] = "CustomQt6Project";

const char mainFilePath[] = "MainFilePath";
const char customImportPaths[] = "CustomImportPaths";
const char canonicalProjectDir[] ="CanonicalProjectDir";

const char enviromentLaunchedQDS[] = "QTC_LAUNCHED_QDS";

const char ALWAYS_OPEN_UI_MODE[] = "J.QtQuick/QmlJSEditor.openUiQmlMode";
const char QML_RESOURCE_PATH[] = "qmldesigner/propertyEditorQmlSources/imports";
const char LANDING_PAGE_PATH[] = "qmldesigner/landingpage";
} // namespace Constants
} // namespace QmlProjectManager
