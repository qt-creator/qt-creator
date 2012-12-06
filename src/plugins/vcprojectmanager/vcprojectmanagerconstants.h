#ifndef VCPROJECTMANAGERCONSTANTS_H
#define VCPROJECTMANAGERCONSTANTS_H

#include <qglobal.h>

namespace VcProjectManager {
namespace Constants {

const char VCPROJ_MIMETYPE[] = "text/x-vc-project"; // TODO: is this good enough?
const char VC_PROJECT_ID[] = "VcProject.VcProject";
const char VC_PROJECT_CONTEXT[] = "VcProject.ProjectContext";
const char VC_PROJECT_TARGET_ID[] = "VcProject.DefaultVcProjectTarget";
const char VC_PROJECT_BC_ID[] = "VcProject.VcProjectBuildConfiguration";
const char VC_PROJECT_SETTINGS_CATEGORY[] = "K.VcProjectSettingsCategory";
const char VC_PROJECT_SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("ProjectExplorer", "Visual Studio");

const char REPARSE_ACTION_ID[] = "VcProject.ReparseMenuAction";
const char REPARSE_CONTEXT_MENU_ACTION_ID[] = "VcProject.ReparseContextMenuAction";

const char VC_PROJECT_SETTINGS_GROUP[] = "VcProjectSettings";
const char VC_PROJECT_MS_BUILD_INFORMATIONS[] = "VcProject.MsBuildInformations";
const char VC_PROJECT_MS_BUILD_EXECUTABLE[] = "VcProject.MsBuildExecutable";
const char VC_PROJECT_MS_BUILD_EXECUTABLE_VERSION[] = "VcProject.MsBuildExecutableVersion";

//const char ACTION_ID[] = "VcProjectManager.Action";
//const char MENU_ID[] = "VcProjectManager.Menu";

} // namespace VcProjectManager
} // namespace Constants

#endif // VCPROJECTMANAGERCONSTANTS_H

