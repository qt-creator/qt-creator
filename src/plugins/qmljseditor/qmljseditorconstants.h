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

#ifndef QMLJSEDITOR_CONSTANTS_H
#define QMLJSEDITOR_CONSTANTS_H

#include <QtCore/QtGlobal>

namespace QmlJSEditor {
namespace Constants {

const char M_CONTEXT[] = "QML JS Editor.ContextMenu";

const char SEPARATOR1[] = "QmlJSEditor.Separator1";
const char SEPARATOR2[] = "QmlJSEditor.Separator2";
const char SEPARATOR3[] = "QmlJSEditor.Separator3";
const char M_REFACTORING_MENU_INSERTION_POINT[] = "QmlJSEditor.RefactorGroup";

const char C_QMLJSEDITOR_ID[] = "QMLProjectManager.QMLJSEditor";
const char C_QMLJSEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "QMLJS Editor");
const char TASK_SEARCH[] = "QmlJSEditor.TaskSearch";
const char SETTINGS_CATEGORY_QML[] = "J.QtQuick";
const char SETTINGS_TR_CATEGORY_QML[] = QT_TRANSLATE_NOOP("QmlJSEditor", "Qt Quick");

const char FOLLOW_SYMBOL_UNDER_CURSOR[] = "QmlJSEditor.FollowSymbolUnderCursor";
const char FIND_USAGES[] = "QmlJSEditor.FindUsages";
const char RENAME_USAGES[] = "QmlJSEditor.RenameUsages";
const char RUN_SEMANTIC_SCAN[] = "QmlJSEditor.RunSemanticScan";
const char REFORMAT_FILE[] = "QmlJSEditor.ReformatFile";
const char SHOW_QT_QUICK_HELPER[] = "QmlJSEditor.ShowQtQuickHelper";

const char TASK_CATEGORY_QML[] = "Task.Category.Qml";
const char TASK_CATEGORY_QML_ANALYSIS[] = "Task.Category.QmlAnalysis";

const char WIZARD_CATEGORY_QML[] = "S.Qml";
const char WIZARD_TR_CATEGORY_QML[] = QT_TRANSLATE_NOOP("QmlJsEditor", "QML");

const char QML_SNIPPETS_GROUP_ID[] = "QML";

} // namespace Constants
} // namespace QmlJSEditor

#endif // QMLJSEDITOR_CONSTANTS_H
