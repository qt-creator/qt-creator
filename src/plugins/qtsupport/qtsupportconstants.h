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

namespace QtSupport {
namespace Constants {

// Qt settings pages
const char QTVERSION_SETTINGS_PAGE_ID[] = "H.Qt Versions";
const char QTVERSION_SETTINGS_PAGE_NAME[] = QT_TRANSLATE_NOOP("QtSupport", "Qt Versions");

const char CODEGEN_SETTINGS_PAGE_ID[] = "Class Generation";
const char CODEGEN_SETTINGS_PAGE_NAME[] = QT_TRANSLATE_NOOP("QtSupport", "Qt Class Generation");

// QtVersions
const char DESKTOPQT[]   = "Qt4ProjectManager.QtVersion.Desktop";
const char WINCEQT[]     = "Qt4ProjectManager.QtVersion.WinCE";

// BaseQtVersion settings
static const char QTVERSIONID[] = "Id";
static const char QTVERSIONNAME[] = "Name";

//Qt Features
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
const char FEATURE_QT_CONSOLE[] = "QtSupport.Wizards.FeatureQtConsole";
const char FEATURE_MOBILE[] = "QtSupport.Wizards.FeatureMobile";
const char FEATURE_DESKTOP[] = "QtSupport.Wizards.FeatureDesktop";

} // namepsace Constants
} // namepsace QtSupport
