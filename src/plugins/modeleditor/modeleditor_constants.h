/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

namespace ModelEditor {
namespace Constants {

const char MODEL_EDITOR_ID[] = "Editors.ModelEditor";
const char MODEL_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Model Editor");

const char REMOVE_SELECTED_ELEMENTS[] = "ModelEditor.RemoveSelectedElements";
const char DELETE_SELECTED_ELEMENTS[] = "ModelEditor.DeleteSelectedElements";
const char OPEN_PARENT_DIAGRAM[] = "ModelEditor.OpenParentDiagram";
const char MENU_ID[] = "ModelEditor.Menu";
const char EXPORT_DIAGRAM[] = "ModelEditor.ExportDiagram";
const char ZOOM_IN[] = "ModelEditor.ZoomIn";
const char ZOOM_OUT[] = "ModelEditor.ZoomOut";
const char RESET_ZOOM[] = "ModelEditor.ResetZoom";
const char ACTION_ADD_PACKAGE[] = "ModelEditor.Action.AddPackage";
const char ACTION_ADD_COMPONENT[] = "ModelEditor.Action.AddComponent";
const char ACTION_ADD_CLASS[] = "ModelEditor.Action.AddClass";
const char ACTION_ADD_CANVAS_DIAGRAM[] = "ModelEditor.Action.AddCanvasDiagram";
const char ACTION_SYNC_BROWSER[] = "ModelEditor.Action.SynchronizeBrowser";

const char EXPLORER_GROUP_MODELING[] = "ModelEditor.ProjectFolder.Group.Modeling";
const char ACTION_EXPLORER_OPEN_DIAGRAM[] = "ModelEditor.Action.Explorer.OpenDiagram";

const char SHORTCUT_MODEL_EDITOR_EDIT_PROPERTIES[] =
        "ModelEditor.ModelEditor.Shortcut.EditProperties";
const char SHORTCUT_MODEL_EDITOR_EDIT_ITEM[] =
        "ModelEditor.ModelEditor.Shortcut.EditItem";

const char WIZARD_CATEGORY[] = "O.Model";
const char WIZARD_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Modeling", "Modeling");
const char WIZARD_MODEL_ID[] = "SA.Model";

const char MIME_TYPE_MODEL[] = "text/vnd.qtcreator.model";

// Settings entries
const char SETTINGS_GROUP[] = "ModelEditorPlugin";
const char SETTINGS_RIGHT_SPLITTER[] = "RightSplitter";
const char SETTINGS_RIGHT_HORIZ_SPLITTER[] = "RightHorizSplitter";

} // namespace Constants
} // namespace ModelEditor
