// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QtGlobal>

namespace Nim {
namespace Constants {

inline constexpr char C_NIMPROJECT_ID[] = "Nim.NimProject";
inline constexpr char C_NIMBLEPROJECT_ID[] = "Nim.NimbleProject";
inline constexpr char C_NIMEDITOR_ID[] = "Nim.NimEditor";

// NimToolchain
inline constexpr char C_NIMTOOLCHAIN_TYPEID[] = "Nim.NimToolChain";

// NimProject
inline constexpr char C_NIMPROJECT_EXCLUDEDFILES[] = "Nim.NimProjectExcludedFiles";

// NimBuildConfiguration
inline constexpr char C_NIMBUILDCONFIGURATION_ID[] = "Nim.NimBuildConfiguration";
inline constexpr char C_NIMBLEBUILDCONFIGURATION_ID[] = "Nim.NimbleBuildConfiguration";
inline constexpr char C_NIMBLEBUILDCONFIGURATION_BUILDTYPE[] = "Nim.NimbleBuildConfiguration.BuildType";

// NimbleBuildStep
inline constexpr char C_NIMBLEBUILDSTEP_ID[] = "Nim.NimbleBuildStep";
inline constexpr char C_NIMBLEBUILDSTEP_ARGUMENTS[] = "Nim.NimbleBuildStep.Arguments";

// NimbleTaskStep
inline constexpr char C_NIMBLETASKSTEP_ID[] = "Nim.NimbleTaskStep";
inline constexpr char C_NIMBLETASKSTEP_TASKNAME[] = "Nim.NimbleTaskStep.TaskName";
inline constexpr char C_NIMBLETASKSTEP_TASKARGS[] = "Nim.NimbleTaskStep.TaskArgs";

// NimCompilerBuildStep
inline constexpr char C_NIMCOMPILERBUILDSTEP_ID[] = "Nim.NimCompilerBuildStep";
inline constexpr char C_NIMCOMPILERBUILDSTEP_USERCOMPILEROPTIONS[] = "Nim.NimCompilerBuildStep.UserCompilerOptions";
inline constexpr char C_NIMCOMPILERBUILDSTEP_DEFAULTBUILDOPTIONS[] = "Nim.NimCompilerBuildStep.DefaultBuildOptions";
inline constexpr char C_NIMCOMPILERBUILDSTEP_TARGETNIMFILE[] = "Nim.NimCompilerBuildStep.TargetNimFile";

// NimCompilerCleanStep
inline constexpr char C_NIMCOMPILERCLEANSTEP_ID[] = "Nim.NimCompilerCleanStep";

inline constexpr char C_NIMLANGUAGE_ID[] = "Nim";
inline constexpr char C_NIMCODESTYLESETTINGSPAGE_ID[] = "Nim.NimCodeStyleSettings";
inline constexpr char C_NIMCODESTYLESETTINGSPAGE_CATEGORY[] = "Z.Nim";

inline constexpr char C_NIMTOOLSSETTINGSPAGE_ID[] = "Nim.NimToolsSettings";
inline constexpr char C_NIMTOOLSSETTINGSPAGE_CATEGORY[] = "Z.Nim";

inline constexpr char C_NIMLANGUAGE_NAME[] = QT_TRANSLATE_NOOP("QtC::Nim", "Nim");
inline constexpr char C_NIMGLOBALCODESTYLE_ID[] = "NimGlobal";
const QString C_NIMSNIPPETSGROUP_ID = QStringLiteral("Nim.NimSnippetsGroup");

inline constexpr char C_NIMCODESTYLEPREVIEWSNIPPET[] =
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
inline constexpr char C_NIM_MIMETYPE[] = "text/x-nim";
inline constexpr char C_NIMBLE_MIMETYPE[] = "text/x-nimble";
inline constexpr char C_NIM_SCRIPT_MIMETYPE[] = "text/x-nim-script";
inline constexpr char C_NIM_MIME_ICON[] = "text-x-nim";
inline constexpr char C_NIM_PROJECT_MIMETYPE[] = "text/x-nim-project";

}
}
