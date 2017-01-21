/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include <QObject>
#include <QtGlobal>

namespace Nim {
namespace Constants {

const char C_NIMPROJECT_ID[] = "Nim.NimProject";
const char C_NIMEDITOR_ID[] = "Nim.NimEditor";
const char C_EDITOR_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("OpenWith::Editors", "Nim Editor");
const char C_NIM_ICON_PATH[] = ":/images/nim.png";

// NimToolChain
const char C_NIMTOOLCHAIN_TYPEID[] = "Nim.NimToolChain";
const char C_NIMTOOLCHAIN_COMPILER_COMMAND_KEY[] = "Nim.NimToolChain.CompilerCommand";

// NimRunConfiguration
const char C_NIMRUNCONFIGURATION_ID[] = "Nim.NimRunConfiguration";
const char C_NIMRUNCONFIGURATION_DISPLAY[] = QT_TRANSLATE_NOOP("NimRunConfiguration", "Current Build Target");
const char C_NIMRUNCONFIGURATION_DEFAULT_DISPLAY[] = QT_TRANSLATE_NOOP("NimRunConfiguration", "Current Build Target");
const QString C_NIMRUNCONFIGURATION_EXECUTABLE_KEY = QStringLiteral("Nim.NimRunConfiguration.Executable");
const QString C_NIMRUNCONFIGURATION_WORKINGDIRECTORY_KEY = QStringLiteral("Nim.NimRunConfiguration.WorkingDirectory");
const QString C_NIMRUNCONFIGURATION_COMMANDLINEARGS_KEY = QStringLiteral("Nim.NimRunConfiguration.CommandlineArgs");
const QString C_NIMRUNCONFIGURATION_RUNMODE_KEY = QStringLiteral("Nim.NimRunConfiguration.RunMode");
const QString C_NIMRUNCONFIGURATION_WORKINGDIRECTORYASPECT_ID = QStringLiteral("Nim.NimRunConfiguration.WorkingDirectoryAspect");
const QString C_NIMRUNCONFIGURATION_ARGUMENTASPECT_ID = QStringLiteral("Nim.NimRunConfiguration.ArgumentAspect");
const QString C_NIMRUNCONFIGURATION_TERMINALASPECT_ID = QStringLiteral("Nim.NimRunConfiguration.TerminalAspect");

// NimProject
const char C_NIMPROJECT_EXCLUDEDFILES[] = "Nim.NimProjectExcludedFiles";

// NimBuildConfiguration
const char C_NIMBUILDCONFIGURATION_ID[] = "Nim.NimBuildConfiguration";
const QString C_NIMBUILDCONFIGURATION_DISPLAY_KEY = QStringLiteral("Nim.NimBuildConfiguration.Display");
const QString C_NIMBUILDCONFIGURATION_BUILDDIRECTORY_KEY = QStringLiteral("Nim.NimBuildConfiguration.BuildDirectory");

// NimBuildConfigurationWidget
const char C_NIMBUILDCONFIGURATIONWIDGET_DISPLAY[] = QT_TRANSLATE_NOOP("NimBuildConfigurationWidget","General");

// NimCompilerBuildStep
const char C_NIMCOMPILERBUILDSTEP_ID[] = "Nim.NimCompilerBuildStep";
const char C_NIMCOMPILERBUILDSTEP_DISPLAY[] = QT_TRANSLATE_NOOP("NimCompilerBuildStep", "Nim Compiler Build Step");
const QString C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS = QStringLiteral("Nim.NimCompilerBuildStep.UserCompilerOptions");
const QString C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS = QStringLiteral("Nim.NimCompilerBuildStep.DefaultBuildOptions");
const QString C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE = QStringLiteral("Nim.NimCompilerBuildStep.TargetNimFile");

// NimCompilerBuildStepWidget
const char C_NIMCOMPILERBUILDSTEPWIDGET_DISPLAY[] = QT_TRANSLATE_NOOP("NimCompilerBuildStepConfigWidget", "Nim build step");
const char C_NIMCOMPILERBUILDSTEPWIDGET_SUMMARY[] = QT_TRANSLATE_NOOP("NimCompilerBuildStepConfigWidget", "Nim build step");

// NimCompilerCleanStep
const char C_NIMCOMPILERCLEANSTEP_ID[] = "Nim.NimCompilerCleanStep";
const char C_NIMCOMPILERCLEANSTEP_DISPLAY[] = QT_TRANSLATE_NOOP("NimCompilerCleanStepFactory", "Nim Compiler Clean Step");

// NimCompilerCleanStepWidget
const char C_NIMCOMPILERCLEANSTEPWIDGET_DISPLAY[] = QT_TRANSLATE_NOOP("NimCompilerCleanStepWidget", "Nim clean step");
const char C_NIMCOMPILERCLEANSTEPWIDGET_SUMMARY[] = QT_TRANSLATE_NOOP("NimCompilerCleanStepWidget", "Nim clean step");

const char C_NIMLANGUAGE_ID[] = "Nim";
const char C_NIMCODESTYLESETTINGSPAGE_ID[] = "Nim.NimCodeStyleSettings";
const char C_NIMCODESTYLESETTINGSPAGE_DISPLAY[] = QT_TRANSLATE_NOOP("NimCodeStyleSettingsPage", "Code Style");
const char C_NIMCODESTYLESETTINGSPAGE_CATEGORY[] = "Z.Nim";
const char C_NIMCODESTYLESETTINGSPAGE_CATEGORY_DISPLAY[] = QT_TRANSLATE_NOOP("NimCodeStyleSettingsPage", "Nim");
const char C_NIMLANGUAGE_NAME[] = QT_TRANSLATE_NOOP("NimCodeStylePreferencesFactory", "Nim");
const char C_NIMGLOBALCODESTYLE_ID[] = "NimGlobal";
const QString C_NIMSNIPPETSGROUP_ID = QStringLiteral("Nim.NimSnippetsGroup");
const char C_NIMSNIPPETPROVIDER_DISPLAY[] = QT_TRANSLATE_NOOP("NimSnippetProvider", "Nim");

const char C_NIMCODESTYLEPREVIEWSNIPPET[] =
        "import os\n"
        "\n"
        "type Foo = ref object of RootObj\n"
        "  name: string\n"
        "  value: int \n"
        "\n"
        "proc newFoo(): Foo =\n"
        "  new(result)\n"
        "\n"
        "if isMainModule():\n"
        "  let foo = newFoo()\n"
        "  echo foo.name\n";

/*******************************************************************************
 * MIME type
 ******************************************************************************/
const char C_NIM_MIMETYPE[] = "text/x-nim";
const char C_NIM_SCRIPT_MIMETYPE[] = "text/x-nim-script";
const char C_NIM_MIME_ICON[] = "text-x-nim";
const char C_NIM_PROJECT_MIMETYPE[] = "text/x-nim-project";

}
}
