// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QtGlobal>

namespace Nim {
namespace Constants {

const char C_NIMPROJECT_ID[] = "Nim.NimProject";
const char C_NIMBLEPROJECT_ID[] = "Nim.NimbleProject";
const char C_NIMEDITOR_ID[] = "Nim.NimEditor";

// NimToolChain
const char C_NIMTOOLCHAIN_TYPEID[] = "Nim.NimToolChain";

// NimProject
const char C_NIMPROJECT_EXCLUDEDFILES[] = "Nim.NimProjectExcludedFiles";

// NimBuildConfiguration
const char C_NIMBUILDCONFIGURATION_ID[] = "Nim.NimBuildConfiguration";
const char C_NIMBLEBUILDCONFIGURATION_ID[] = "Nim.NimbleBuildConfiguration";
const char C_NIMBLEBUILDCONFIGURATION_BUILDTYPE[] = "Nim.NimbleBuildConfiguration.BuildType";

// NimbleBuildStep
const char C_NIMBLEBUILDSTEP_ID[] = "Nim.NimbleBuildStep";
const char C_NIMBLEBUILDSTEP_ARGUMENTS[] = "Nim.NimbleBuildStep.Arguments";

// NimbleTaskStep
const char C_NIMBLETASKSTEP_ID[] = "Nim.NimbleTaskStep";
const char C_NIMBLETASKSTEP_TASKNAME[] = "Nim.NimbleTaskStep.TaskName";
const char C_NIMBLETASKSTEP_TASKARGS[] = "Nim.NimbleTaskStep.TaskArgs";

// NimCompilerBuildStep
const char C_NIMCOMPILERBUILDSTEP_ID[] = "Nim.NimCompilerBuildStep";
const char C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS[] = "Nim.NimCompilerBuildStep.UserCompilerOptions";
const char C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS[] = "Nim.NimCompilerBuildStep.DefaultBuildOptions";
const char C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE[] = "Nim.NimCompilerBuildStep.TargetNimFile";

// NimCompilerCleanStep
const char C_NIMCOMPILERCLEANSTEP_ID[] = "Nim.NimCompilerCleanStep";

const char C_NIMLANGUAGE_ID[] = "Nim";
const char C_NIMCODESTYLESETTINGSPAGE_ID[] = "Nim.NimCodeStyleSettings";
const char C_NIMCODESTYLESETTINGSPAGE_CATEGORY[] = "Z.Nim";

const char C_NIMTOOLSSETTINGSPAGE_ID[] = "Nim.NimToolsSettings";
const char C_NIMTOOLSSETTINGSPAGE_CATEGORY[] = "Z.Nim";

const char C_NIMLANGUAGE_NAME[] = QT_TRANSLATE_NOOP("QtC::Nim", "Nim");
const char C_NIMGLOBALCODESTYLE_ID[] = "NimGlobal";
const QString C_NIMSNIPPETSGROUP_ID = QStringLiteral("Nim.NimSnippetsGroup");

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
const char C_NIMBLE_MIMETYPE[] = "text/x-nimble";
const char C_NIM_SCRIPT_MIMETYPE[] = "text/x-nim-script";
const char C_NIM_MIME_ICON[] = "text-x-nim";
const char C_NIM_PROJECT_MIMETYPE[] = "text/x-nim-project";

}
}
