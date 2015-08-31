/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
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

#ifndef MODELEDITORCONSTANTS_H
#define MODELEDITORCONSTANTS_H

namespace ModelEditor {
namespace Constants {

const char MODEL_EDITOR_ID[] = "Editors.ModelEditor";
const char MODEL_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Model Editor");

const char DIAGRAM_EDITOR_ID[] = "Editors.DiagramEditor";
const char DIAGRAM_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Model Editor");

const char REMOVE_SELECTED_ELEMENTS[] = "ModelEditor.RemoveSelectedElements";
const char DELETE_SELECTED_ELEMENTS[] = "ModelEditor.DeleteSelectedElements";
const char ACTION_ADD_PACKAGE[] = "ModelEditor.Action.AddPackage";
const char ACTION_ADD_COMPONENT[] = "ModelEditor.Action.AddComponent";
const char ACTION_ADD_CLASS[] = "ModelEditor.Action.AddClass";
const char ACTION_ADD_CANVAS_DIAGRAM[] = "ModelEditor.Action.AddCanvasDiagram";

const char EXPLORER_GROUP_MODELING[] = "ModelEditor.ProjectFolder.Group.Modeling";
const char ACTION_EXPLORER_OPEN_DIAGRAM[] = "ModelEditor.Action.Explorer.OpenDiagram";

const char SHORTCUT_MODEL_EDITOR_EDIT_PROPERTIES[] =
        "ModelEditor.ModelEditor.Shortcut.EditProperties";
const char SHORTCUT_DIAGRAM_EDITOR_EDIT_PROPERTIES[] =
        "ModelEditor.DiagramEditor.Shortcut.EditProperties";

const char WIZARD_CATEGORY[] = "O.Model";
const char WIZARD_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Modeling", "Modeling");
const char WIZARD_MODEL_ID[] = "SA.Model";

const char MIME_TYPE_MODEL[] = "text/vnd.qtcreator.model";
const char MIME_TYPE_DIAGRAM_REFERENCE[] = "text/vnd.qtcreator.diagram-reference";

// Settings entries
const char SETTINGS_GROUP[] = "ModelEditorPlugin";
const char SETTINGS_RIGHT_SPLITTER[] = "RightSplitter";
const char SETTINGS_RIGHT_HORIZ_SPLITTER[] = "RightHorizSplitter";

} // namespace Constants
} // namespace ModelEditor

#endif // MODELEDITORCONSTANTS_H
