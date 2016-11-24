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

#include <QtGlobal>

namespace QmlJSEditor {
namespace Constants {

const char QML_JS_EDITOR_PLUGIN[] = "QmlJSEditorPlugin";

const char M_CONTEXT[] = "QML JS Editor.ContextMenu";

const char M_REFACTORING_MENU_INSERTION_POINT[] = "QmlJSEditor.RefactorGroup";

const char C_QMLJSEDITOR_ID[] = "QMLProjectManager.QMLJSEditor";
const char C_QMLJSEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "QMLJS Editor");
const char TASK_SEARCH[] = "QmlJSEditor.TaskSearch";
const char SETTINGS_CATEGORY_QML[] = "J.QtQuick";
const char SETTINGS_TR_CATEGORY_QML[] = QT_TRANSLATE_NOOP("QmlJSEditor", "Qt Quick");

const char FIND_USAGES[] = "QmlJSEditor.FindUsages";
const char RENAME_USAGES[] = "QmlJSEditor.RenameUsages";
const char RUN_SEMANTIC_SCAN[] = "QmlJSEditor.RunSemanticScan";
const char REFORMAT_FILE[] = "QmlJSEditor.ReformatFile";
const char SHOW_QT_QUICK_HELPER[] = "QmlJSEditor.ShowQtQuickHelper";
const char INSPECT_ELEMENT_UNDER_CURSOR[] = "QmlJSEditor.InspectElementUnderCursor";

const char TASK_CATEGORY_QML[] = "Task.Category.Qml";
const char TASK_CATEGORY_QML_ANALYSIS[] = "Task.Category.QmlAnalysis";

const char QML_SNIPPETS_GROUP_ID[] = "QML";

const char QML_CONTEXTPANE_KEY[] = "QmlJSEditor.ContextPaneEnabled";
const char QML_CONTEXTPANEPIN_KEY[] = "QmlJSEditor.ContextPanePinned";

const char QML_UI_FILE_WARNING[] = "QmlJSEditor.QmlUiFileWarning";

const char AUTO_FORMAT_ON_SAVE[] = "QmlJSEditor.AutoFormatOnSave";
const char AUTO_FORMAT_ONLY_CURRENT_PROJECT[] = "QmlJSEditor.AutoFormatOnlyCurrentProject";

} // namespace Constants
} // namespace QmlJSEditor
