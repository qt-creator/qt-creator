/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSEDITOR_CONSTANTS_H
#define QMLJSEDITOR_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace QmlJSEditor {
namespace Constants {

// menus
const char * const M_CONTEXT = "QML JS Editor.ContextMenu";
const char * const M_TOOLS_QML = "QmlJSEditor.Tools.Menu";

const char * const SEPARATOR1 = "QmlJSEditor.Separator1";
const char * const SEPARATOR2 = "QmlJSEditor.Separator2";
const char * const M_REFACTORING_MENU_INSERTION_POINT = "QmlJSEditor.RefactorGroup";

const char * const RUN_SEP = "QmlJSEditor.Run.Separator";
const char * const C_QMLJSEDITOR_ID = "QMLProjectManager.QMLJSEditor";
const char * const C_QMLJSEDITOR_DISPLAY_NAME = QT_TRANSLATE_NOOP("OpenWith::Editors", "QMLJS Editor");
const char * const TASK_SEARCH = "QmlJSEditor.TaskSearch";

const char * const FOLLOW_SYMBOL_UNDER_CURSOR = "QmlJSEditor.FollowSymbolUnderCursor";
const char * const FIND_USAGES = "QmlJSEditor.FindUsages";
const char * const SHOW_QT_QUICK_HELPER = "QmlJSEditor.ShowQtQuickHelper";

const char * const QML_MIMETYPE = "application/x-qml";
const char * const JS_MIMETYPE = "application/javascript";

const char *const TASK_CATEGORY_QML = "Task.Category.Qml";

const char * const WIZARD_CATEGORY_QML = "S.Qml";
const char * const WIZARD_TR_CATEGORY_QML = QT_TRANSLATE_NOOP("QmlJsEditor", "QML");

const char * const QML_SNIPPETS_GROUP_ID = "QML";

} // namespace Constants
} // namespace QmlJSEditor

#endif // QMLJSEDITOR_CONSTANTS_H
