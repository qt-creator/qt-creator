// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace QtSupport {
namespace Constants {

// Qt settings pages
const char QTVERSION_SETTINGS_PAGE_ID[] = "H.Qt Versions";
const char CODEGEN_SETTINGS_PAGE_ID[] = "Class Generation";

// QtVersions
const char DESKTOPQT[]   = "Qt4ProjectManager.QtVersion.Desktop";

// QtVersion settings
static const char QTVERSIONID[] = "Id";
static const char QTVERSIONNAME[] = "Name";

// Qt Features
const char FEATURE_QT_PREFIX[] = "QtSupport.Wizards.FeatureQt";
const char FEATURE_QWIDGETS[] = "QtSupport.Wizards.FeatureQWidgets";
const char FEATURE_QT_QUICK_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick";
const char FEATURE_QMLPROJECT[] = "QtSupport.Wizards.FeatureQtQuickProject";
const char FEATURE_QT_QUICK_CONTROLS_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick.Controls";
const char FEATURE_QT_QUICK_CONTROLS_2_PREFIX[] = "QtSupport.Wizards.FeatureQtQuick.Controls.2";
const char FEATURE_QT_LABS_CONTROLS_PREFIX[] = "QtSupport.Wizards.FeatureQt.labs.controls";
const char FEATURE_QT_QUICK_UI_FILES[] = "QtSupport.Wizards.FeatureQtQuick.UiFiles";
const char FEATURE_QT_WEBKIT[] = "QtSupport.Wizards.FeatureQtWebkit";
const char FEATURE_QT_3D[] = "QtSupport.Wizards.FeatureQt3d";
const char FEATURE_QT_CANVAS3D_PREFIX[] = "QtSupport.Wizards.FeatureQtCanvas3d";
const char FEATURE_QT_QML_CMAKE_API[] = "QtSupport.Wizards.FeatureQtQmlCMakeApi";
const char FEATURE_QT_CONSOLE[] = "QtSupport.Wizards.FeatureQtConsole";
const char FEATURE_MOBILE[] = "QtSupport.Wizards.FeatureMobile";
const char FEATURE_DESKTOP[] = "QtSupport.Wizards.FeatureDesktop";

// Kit flags
const char FLAGS_SUPPLIES_QTQUICK_IMPORT_PATH[] = "QtSupport.SuppliesQtQuickImportPath";
const char KIT_QML_IMPORT_PATH[] = "QtSupport.KitQmlImportPath";
const char KIT_HAS_MERGED_HEADER_PATHS_WITH_QML_IMPORT_PATHS[] =
        "QtSupport.KitHasMergedHeaderPathsWithQmlImportPaths";

} // namepsace Constants
} // namepsace QtSupport
