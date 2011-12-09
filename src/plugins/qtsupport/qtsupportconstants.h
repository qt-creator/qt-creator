/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QTSUPPORTCONSTANTS_H
#define QTSUPPORTCONSTANTS_H

namespace QtSupport {
namespace Constants {

// Qt4 settings pages
const char QTVERSION_SETTINGS_PAGE_ID[] = "B.Qt Versions";
const char QTVERSION_SETTINGS_PAGE_NAME[] = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt Versions");

// QtVersions
const char SYMBIANQT[]   = "Qt4ProjectManager.QtVersion.Symbian";
const char MAEMOQT[]     = "Qt4ProjectManager.QtVersion.Maemo";
const char DESKTOPQT[]   = "Qt4ProjectManager.QtVersion.Desktop";
const char SIMULATORQT[] = "Qt4ProjectManager.QtVersion.Simulator";
const char WINCEQT[]     = "Qt4ProjectManager.QtVersion.WinCE";

// QML wizard categories
// both the qt4projectmanager and the qmlprojectmanager do have qt quick wizards
// so we define the category here
const char QML_WIZARD_CATEGORY[] = "C.Projects"; // (before Qt)
const char QML_WIZARD_TR_SCOPE[] = "QmlProjectManager";
const char QML_WIZARD_TR_CATEGORY[] = QT_TRANSLATE_NOOP("QmlProjectManager", "Qt Quick Project");
const char QML_WIZARD_ICON[] = ":/qmlproject/images/qml_wizard.png";

} // namepsace Constants
} // namepsace QtSupport

#endif // QTSUPPORTCONSTANTS_H
