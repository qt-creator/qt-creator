/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTSUPPORTCONSTANTS_H
#define QTSUPPORTCONSTANTS_H

namespace QtSupport {
namespace Constants {

// Qt4 settings pages
const char QTVERSION_SETTINGS_PAGE_ID[] = "H.Qt Versions";
const char QTVERSION_SETTINGS_PAGE_NAME[] = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt Versions");

// QtVersions
const char MAEMOQT[]     = "Qt4ProjectManager.QtVersion.Maemo";
const char DESKTOPQT[]   = "Qt4ProjectManager.QtVersion.Desktop";
const char SIMULATORQT[] = "Qt4ProjectManager.QtVersion.Simulator";
const char WINCEQT[]     = "Qt4ProjectManager.QtVersion.WinCE";

//Qt Features
const char FEATURE_QT[] = "QtSupport.Wizards.FeatureQt";
const char FEATURE_QWIDGETS[] = "QtSupport.Wizards.FeatureQWidgets";
const char FEATURE_QT_QUICK[] = "QtSupport.Wizards.FeatureQtQuick";
const char FEATURE_QMLPROJECT[] = "QtSupport.Wizards.FeatureQtQuickProject";
const char FEATURE_QT_QUICK_1[] = "QtSupport.Wizards.FeatureQtQuick.1";
const char FEATURE_QT_QUICK_1_1[] = "QtSupport.Wizards.FeatureQtQuick.1.1";
const char FEATURE_QT_QUICK_2[] = "QtSupport.Wizards.FeatureQtQuick.2";
const char FEATURE_QT_WEBKIT[] = "QtSupport.Wizards.FeatureQtWebkit";
const char FEATURE_QT_CONSOLE[] = "QtSupport.Wizards.FeatureQtConsole";
const char FEATURE_QTQUICK_COMPONENTS_MEEGO[] = "QtSupport.Wizards.FeatureQtQuickComponentsMeego";
const char FEATURE_MOBILE[] = "QtSupport.Wizards.FeatureMobile";
const char FEATURE_DESKTOP[] = "QtSupport.Wizards.FeatureDesktop";

// Platforms
const char MEEGO_HARMATTAN_PLATFORM[] = "MeeGo/Harmattan";
const char MAEMO_FREMANTLE_PLATFORM[] = "Maemo/Fremantle";
const char DESKTOP_PLATFORM[] = "Desktop";
const char EMBEDDED_LINUX_PLATFORM[] = "Embedded Linux";
const char WINDOWS_CE_PLATFORM[] = "Windows CE";
const char ANDROID_PLATFORM[] = "Android";

const char MEEGO_HARMATTAN_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "MeeGo/Harmattan");
const char MAEMO_FREMANTLE_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Maemo/Fremantle");
const char DESKTOP_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Desktop");
const char EMBEDDED_LINUX_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Embedded Linux");
const char WINDOWS_CE_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Windows CE");
const char ANDROID_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Android");

// QML wizard icon
// both the qt4projectmanager and the qmlprojectmanager do have qt quick wizards
// so we define the icon here
const char QML_WIZARD_ICON[] = ":/qmlproject/images/qml_wizard.png";

} // namepsace Constants
} // namepsace QtSupport

#endif // QTSUPPORTCONSTANTS_H
