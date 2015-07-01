/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTSUPPORTCONSTANTS_H
#define QTSUPPORTCONSTANTS_H

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
const char FEATURE_QT_QUICK_UI_FILES[] = "QtSupport.Wizards.FeatureQtQuick.UiFiles";
const char FEATURE_QT_WEBKIT[] = "QtSupport.Wizards.FeatureQtWebkit";
const char FEATURE_QT_3D[] = "QtSupport.Wizards.FeatureQt3d";
const char FEATURE_QT_CANVAS3D[] = "QtSupport.Wizards.FeatureQtCanvas3d";
const char FEATURE_QT_CONSOLE[] = "QtSupport.Wizards.FeatureQtConsole";
const char FEATURE_MOBILE[] = "QtSupport.Wizards.FeatureMobile";
const char FEATURE_DESKTOP[] = "QtSupport.Wizards.FeatureDesktop";

// Platforms
const char DESKTOP_PLATFORM[] = "Desktop";
const char EMBEDDED_LINUX_PLATFORM[] = "Embedded Linux";
const char WINDOWS_CE_PLATFORM[] = "Windows CE";
const char WINDOWS_RT_PLATFORM[] = "Windows Runtime";
const char WINDOWS_PHONE_PLATFORM[] = "Windows Phone";
const char ANDROID_PLATFORM[] = "Android";
const char IOS_PLATFORM[] = "iOS";

const char DESKTOP_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Desktop");
const char EMBEDDED_LINUX_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Embedded Linux");
const char WINDOWS_CE_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Windows CE");
const char WINDOWS_RT_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Windows Runtime");
const char WINDOWS_PHONE_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Windows Phone");
const char ANDROID_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Android");
const char IOS_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "iOS");

const char ICON_QT_PROJECT[] = ":/qtsupport/images/qt_project.png";

} // namepsace Constants
} // namepsace QtSupport

#endif // QTSUPPORTCONSTANTS_H
