// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace MesonProjectManager::Constants {

namespace Project {
inline constexpr char MIMETYPE[] = "text/x-meson";
inline constexpr char ID[] = "MesonProjectManager.MesonProject";
} // namespace Project

namespace BuildConfiguration {
inline constexpr char BUILD_TYPE_KEY[] = "MesonProjectManager.BuildConfig.Type";
inline constexpr char PARAMETERS_KEY[] = "MesonProjectManager.BuildConfig.Parameters";
}

// Settings page
namespace SettingsPage {
inline constexpr char TOOLS_ID[] = "Z.MesonProjectManager.SettingsPage.Tools";
inline constexpr char CATEGORY[] = "Z.Meson";
} // namespace SettingsPage

namespace ToolsSettings {
inline constexpr char FILENAME[] = "mesontools.xml";
inline constexpr char ENTRY_KEY[] = "Tool.";
inline constexpr char ENTRY_COUNT[] = "Tools.Count";
inline constexpr char DEFAULT_TOOL_KEY[] = "MesonTools.Default";
inline constexpr char TOOL_TYPE_KEY[] = "type";
inline constexpr char TOOL_TYPE_MESON[] = "meson";
inline constexpr char EXE_KEY[] = "exe";
inline constexpr char AUTO_DETECTED_KEY[] = "autodetected";
inline constexpr char NAME_KEY[] = "name";
inline constexpr char ID_KEY[] = "uuid";
} // namespace ToolsSettings

namespace Icons {
inline constexpr char MESON[] = ":/mesonproject/icons/meson_logo.png";
inline constexpr char MESON_BW[] = ":/mesonproject/icons/meson_bw_logo.png";
} // namespace Icons

inline constexpr char MESON_INFO_DIR[] = "meson-info";
inline constexpr char MESON_INTRO_BENCHMARKS[] = "intro-benchmarks.json";
inline constexpr char MESON_INTRO_BUIDOPTIONS[] = "intro-buildoptions.json";
inline constexpr char MESON_INTRO_BUILDSYSTEM_FILES[] = "intro-buildsystem_files.json";
inline constexpr char MESON_INTRO_DEPENDENCIES[] = "intro-dependencies.json";
inline constexpr char MESON_INTRO_INSTALLED[] = "intro-installed.json";
inline constexpr char MESON_INTRO_PROJECTINFO[] = "intro-projectinfo.json";
inline constexpr char MESON_INTRO_TARGETS[] = "intro-targets.json";
inline constexpr char MESON_INTRO_TESTS[] = "intro-tests.json";
inline constexpr char MESON_INFO[] = "meson-info.json";

inline constexpr char MESON_TOOL_MANAGER[] = "MesonProjectManager.Tools";
inline constexpr char MESON_BUILD_STEP_ID[] = "MesonProjectManager.BuildStep";

inline constexpr char MESON_RUNCONFIG_ID[] = "MesonProjectManager.MesonRunConfiguration";

namespace Targets {
inline constexpr char all[] = "all";
inline constexpr char clean[] = "clean";
inline constexpr char install[] = "install";
inline constexpr char tests[] = "test";
inline constexpr char benchmark[] = "benchmark";
} // namespace Targets
inline constexpr char MESON_BUILD_CONFIG_ID[] = "MesonProjectManager.BuildConfiguration";

} // namespace MesonProjectManager::Constants
