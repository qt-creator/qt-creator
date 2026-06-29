// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QtSupport::Constants {

// Qt settings pages
inline constexpr char QTVERSION_SETTINGS_PAGE_ID[] = "H.Qt Versions";
inline constexpr char CODEGEN_SETTINGS_PAGE_ID[] = "Class Generation";

// QtVersions
inline constexpr char DESKTOPQT[]   = "Qt4ProjectManager.QtVersion.Desktop";

// QtVersion settings
static const char QTVERSIONID[] = "Id";
static const char QTVERSIONNAME[] = "Name";

// Qt Features
inline constexpr char FEATURE_QT_PREFIX[] = "QtSupport.Wizards.FeatureQt";
inline constexpr char FEATURE_QWIDGETS[] = "QtSupport.Wizards.FeatureQWidgets";
inline constexpr char FEATURE_QT_QUICK_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick";
inline constexpr char FEATURE_QMLPROJECT[] = "QtSupport.Wizards.FeatureQtQuickProject";
inline constexpr char FEATURE_QT_QUICK_CONTROLS_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick.Controls";
inline constexpr char FEATURE_QT_QUICK_CONTROLS_2_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick.Controls.2";
inline constexpr char FEATURE_QT_LABS_CONTROLS_PREFIX[] = "QtSupport.Wizards.FeatureQt.labs.controls";
inline constexpr char FEATURE_QT_QUICK_UI_FILES[] = "QtSupport.Wizards.FeatureQtQuick.UiFiles";
inline constexpr char FEATURE_QT_WEBKIT[] = "QtSupport.Wizards.FeatureQtWebkit";
inline constexpr char FEATURE_QT_3D[] = "QtSupport.Wizards.FeatureQt3d";
inline constexpr char FEATURE_QT_CANVAS3D_PREFIX[] = "QtSupport.Wizards.FeatureQtCanvas3d";
inline constexpr char FEATURE_QT_QML_CMAKE_API[] = "QtSupport.Wizards.FeatureQtQmlCMakeApi";
inline constexpr char FEATURE_QT_CONSOLE[] = "QtSupport.Wizards.FeatureQtConsole";
inline constexpr char FEATURE_MOBILE[] = "QtSupport.Wizards.FeatureMobile";
inline constexpr char FEATURE_DESKTOP[] = "QtSupport.Wizards.FeatureDesktop";

// Kit flags
inline constexpr char FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH[] = "QtSupport.SuppliesQtQuickImportPath";
inline constexpr char KIT_QML_IMPORT_PATH[] = "QtSupport.KitQmlImportPath";
inline constexpr char KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS[] =
        "QtSupport.KitHasMergedHeaderPathsWithQmlImportPaths";

} // namepsace QtSupport::Constants
