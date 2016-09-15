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

#include "scxmleditor_global.h"

namespace ScxmlEditor {
namespace Constants {

const char INFO_READ_ONLY[] = "ScxmlEditor.ReadOnly";

const char C_SCXMLEDITOR[] = "Qt5.ScxmlEditor";
const char C_SCXMLEDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Scxml Editor");

const char K_SCXML_EDITOR_ID[] = "ScxmlEditor.XmlEditor";
const char C_SCXML_EDITOR[] = "Scxml Editor";

const char C_SCXMLTAG_ATTRIBUTE_ID[] = "id";
const char C_SCXMLTAG_ATTRIBUTE_EVENT[] = "event";

const char C_SCXMLTAG_TAGTYPE[] = "tagType";
const char C_SCXMLTAG_ACTIONTYPE[] = "actionType";
const char C_SCXMLTAG_PARENTTAG[] = "parentTag";

const char C_SCXML_EDITORINFO_COLORS[] = "colors";
const char C_SCXML_EDITORINFO_GEOMETRY[] = "geometry";
const char C_SCXML_EDITORINFO_SCENEGEOMETRY[] = "scenegeometry";
const char C_SCXML_EDITORINFO_LOCALGEOMETRY[] = "localGeometry";
const char C_SCXML_EDITORINFO_MOVEPOINT[] = "movePoint";
const char C_SCXML_EDITORINFO_STARTTARGETFACTORS[] = "startTargetFactors";
const char C_SCXML_EDITORINFO_ENDTARGETFACTORS[] = "endTargetFactors";
const char C_SCXML_EDITORINFO_FONTCOLOR[] = "fontColor";
const char C_SCXML_EDITORINFO_STATECOLOR[] = "stateColor";

const char C_STATE_WARNING_ID[] = "IDWarning";
const char C_STATE_WARNING_STATE[] = "StateWarning";
const char C_STATE_WARNING_TRANSITION[] = "TransitionWarning";
const char C_STATE_WARNING_INITIAL[] = "InitialWarning";

const char C_UI_FACTORY_OBJECT_ACTIONPROVIDER[] = "actionProvider";

const char C_COLOR_SCHEME_DEFAULT[] = "factory_default_theme";
const char C_COLOR_SCHEME_SCXMLDOCUMENT[] = "scxmldocument_theme";

const char C_OBJECTNAME_WARNINGPROVIDER[] = "warningProvider";

const char C_WARNINGMODEL_FILTER_ACTIVE[] = "active";
const char C_WARNINGMODEL_FILTER_NOT[] = "not";

const char C_SETTINGS_SPLITTER[] = "ScxmlEditor/HorizontalSplitter";
const char C_SETTINGS_LASTEXPORTFOLDER[] = "ScxmlEditor/LastExportFolder";
const char C_SETTINGS_LASTSAVESCREENSHOTFOLDER[] = "ScxmlEditor/LastSaveScreenshotFolder";
const char C_SETTINGS_COLORSETTINGS_COLORTHEMES[] = "ScxmlEditor/ColorSettingsColorThemes";
const char C_SETTINGS_COLORSETTINGS_CURRENTCOLORTHEME[] = "ScxmlEditor/ColorSettingsCurrentColorTheme";
const char C_SETTINGS_ERRORPANE_GEOMETRY[] = "ScxmlEditor/ErrorPaneGeometry";
const char C_SETTINGS_ERRORPANE_SHOWERRORS[] = "ScxmlEditor/ErrorPaneShowErrors";
const char C_SETTINGS_ERRORPANE_SHOWWARNINGS[] = "ScxmlEditor/ErrorPaneShowWarnings";
const char C_SETTINGS_ERRORPANE_SHOWINFOS[] = "ScxmlEditor/ErrorPaneShowInfos";

} // namespace ScxmlEditor
} // namespace Constants
