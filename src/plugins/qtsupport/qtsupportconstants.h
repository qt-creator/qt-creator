/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QTSUPPORTCONSTANTS_H
#define QTSUPPORTCONSTANTS_H

namespace QtSupport {
namespace Constants {

//Qt4 settings pages
const char * const QT_SETTINGS_CATEGORY       = "L.Qt4";
const char * const QT_SETTINGS_CATEGORY_ICON  = ":/core/images/category_qt.png";
const char * const QT_SETTINGS_TR_CATEGORY    = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt4");
const char * const QTVERSION_SETTINGS_PAGE_ID = "Qt Versions";
const char * const QTVERSION_SETTINGS_PAGE_NAME = QT_TRANSLATE_NOOP("Qt4ProjectManager", "Qt Versions");

// QtVersions
const char * const SYMBIANQT   = "Qt4ProjectManager.QtVersion.Symbian";
const char * const MAEMOQT     = "Qt4ProjectManager.QtVersion.Maemo";
const char * const DESKTOPQT   = "Qt4ProjectManager.QtVersion.Desktop";
const char * const SIMULATORQT = "Qt4ProjectManager.QtVersion.Simulator";
const char * const WINCEQT     = "Qt4ProjectManager.QtVersion.WinCE";

// QML wizard categories
// both the qt4projectmanager and the qmlprojectmanager do have qt quick wizards
// so we define the category here
const char * const QML_WIZARD_CATEGORY = "C.Projects"; // (before Qt)
const char * const QML_WIZARD_TR_SCOPE = "QmlProjectManager";
const char * const QML_WIZARD_TR_CATEGORY = QT_TRANSLATE_NOOP("QmlProjectManager", "Qt Quick Project");
const char * const QML_WIZARD_ICON = ":/qmlproject/images/qml_wizard.png";

}
}
#endif // QTSUPPORTCONSTANTS_H
