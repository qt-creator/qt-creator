// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace MesonProjectManager {
namespace Constants {

namespace Project {
const char MIMETYPE[] = "text/x-meson";
const char ID[] = "MesonProjectManager.MesonProject";
} // namespace Project

namespace BuildConfiguration {
const char BUILD_TYPE_KEY[] = "MesonProjectManager.BuildConfig.Type";
const char PARAMETERS_KEY[] = "MesonProjectManager.BuildConfig.Parameters";
}

// Settings page
namespace SettingsPage {
const char TOOLS_ID[] = "Z.MesonProjectManager.SettingsPage.Tools";
const char CATEGORY[] = "Z.Meson";
} // namespace SettingsPage

namespace ToolsSettings {
const char FILENAME[] = "mesontools.xml";
const char ENTRY_KEY[] = "Tool.";
const char ENTRY_COUNT[] = "Tools.Count";
const char TOOL_TYPE_KEY[] = "type";
const char TOOL_TYPE_MESON[] = "meson";
const char TOOL_TYPE_NINJA[] = "ninja";
const char EXE_KEY[] = "exe";
const char AUTO_DETECTED_KEY[] = "autodetected";
const char NAME_KEY[] = "name";
const char ID_KEY[] = "uuid";
} // namespace ToolsSettings

namespace Icons {
const char MESON[] = ":/mesonproject/icons/meson_logo.png";
const char MESON_BW[] = ":/mesonproject/icons/meson_bw_logo.png";
} // namespace Icons

const char MESON_INFO_DIR[] = "meson-info";
const char MESON_INTRO_BENCHMARKS[] = "intro-benchmarks.json";
const char MESON_INTRO_BUIDOPTIONS[] = "intro-buildoptions.json";
const char MESON_INTRO_BUILDSYSTEM_FILES[] = "intro-buildsystem_files.json";
const char MESON_INTRO_DEPENDENCIES[] = "intro-dependencies.json";
const char MESON_INTRO_INSTALLED[] = "intro-installed.json";
const char MESON_INTRO_PROJECTINFO[] = "intro-projectinfo.json";
const char MESON_INTRO_TARGETS[] = "intro-targets.json";
const char MESON_INTRO_TESTS[] = "intro-tests.json";
const char MESON_INFO[] = "meson-info.json";

const char MESON_TOOL_MANAGER[] = "MesonProjectManager.Tools";
const char MESON_BUILD_STEP_ID[] = "MesonProjectManager.BuildStep";

namespace Targets {
const char all[] = "all";
const char clean[] = "clean";
const char install[] = "install";
const char tests[] = "test";
const char benchmark[] = "benchmark";
const char clang_format[] = "clang-format";
const char scan_build[] = "scan-build";
} // namespace Targets
const char MESON_BUILD_CONFIG_ID[] = "MesonProjectManager.BuildConfiguration";

} // namespace Constants
} // namespace MesonProjectManager
