/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#ifndef VCPROJECTMANAGERCONSTANTS_H
#define VCPROJECTMANAGERCONSTANTS_H

#include <qglobal.h>

namespace VcProjectManager {
namespace Constants {

const char VCPROJ_MIMETYPE[] = "text/x-vc-project"; // TODO: is this good enough?
const char VC_PROJECT_ID[] = "VcProject.VcProject";  // 2003 and 2008 version
const char VC_PROJECT_2005_ID[] = "VcProject.VcProject2005"; // 2005 version
const char VC_PROJECT_CONTEXT[] = "VcProject.ProjectContext";
const char VC_PROJECT_TARGET_ID[] = "VcProject.DefaultVcProjectTarget";
const char VC_PROJECT_BC_ID[] = "VcProject.VcProjectBuildConfiguration";
const char VC_PROJECT_SETTINGS_CATEGORY[] = "K.VcProjectSettingsCategory";
const char VC_PROJECT_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Visual Studio");

const char REPARSE_ACTION_ID[] = "VcProject.ReparseMenuAction";
const char REPARSE_CONTEXT_MENU_ACTION_ID[] = "VcProject.ReparseContextMenuAction";

const char VC_PROJECT_BUILD_CONFIGURATION_NAME[] = "VcProject.ConfigurationName";
const char VC_PROJECT_SETTINGS_GROUP[] = "VcProjectSettings";
const char VC_PROJECT_MS_BUILD_INFORMATIONS[] = "VcProject.MsBuildInformations";
const char VC_PROJECT_MS_BUILD_EXECUTABLE[] = "VcProject.MsBuildExecutable";
const char VC_PROJECT_MS_BUILD_EXECUTABLE_VERSION[] = "VcProject.MsBuildExecutableVersion";
const char VC_PROJECT_MS_BUILD_ARGUMENT_LIST[] = "VcProject.MsBuildArgumentList";
const char VC_PROJECT_SCHEMA_PATH[] = "VcProject.SchemaPath";
const char VC_PROJECT_SCHEMA_2003_QUIALIFIER[] = "Schema2003";
const char VC_PROJECT_SCHEMA_2005_QUIALIFIER[] = "Schema2005";
const char VC_PROJECT_SCHEMA_2008_QUIALIFIER[] = "Schema2008";
const char VC_PROJECT_TOOL_SCHEMA[] = "VcProject.ToolSchema";
const char VC_PROJECT_TOOL_XML[] = "VcProject.ToolXML";
const char VC_PROJECT_KIT_INFO_ID[] = "VcProject.KitInformation";

const char VC_PROJECT_SHOW_PROPERTIES_ACTION[] = "VcProject.ShowPropertiesAction";
const char VC_PROJECT_ADD_FOLDER_ACTION[] = "VcProject.AddFolderAction";
const char VC_PROJECT_ADD_FILTER_ACTION[] = "VcProject.AddFilterAction";
const char VC_PROJECT_REMOVE_FOLDER_ACTION[] = "VcProject.RemoveFolderAction";
const char VC_PROJECT_REMOVE_FILTER_ACTION[] = "VcProject.RemoveFilterAction";

// File container constants
const char VC_PROJECT_FILE_CONTAINER_FOLDER[] = "Folder";
const char VC_PROJECT_FILE_CONTAINER_FILTER[] = "Filter";

enum SchemaVersion
{
    SV_2003,
    SV_2005,
    SV_2008
};
//const char ACTION_ID[] = "VcProjectManager.Action";
//const char MENU_ID[] = "VcProjectManager.Menu";

} // namespace VcProjectManager
} // namespace Constants

#endif // VCPROJECTMANAGERCONSTANTS_H

