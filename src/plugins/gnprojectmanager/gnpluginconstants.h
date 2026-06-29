// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace GNProjectManager::Constants {

namespace Project {

inline constexpr char MIMETYPE[] = "text/x-gn";
} // namespace Project

namespace BuildConfiguration {

inline constexpr char PARAMETERS_KEY[] = "GNProjectManager.BuildConfig.Parameters";

}

inline constexpr char GN_PROJECT_ID[] = "GNProjectManager.GNProject";
inline constexpr char PROJECT_JSON[] = "project.json";
inline constexpr char PROJECT_AGS[] = "args.gn";

inline constexpr char GN_BUILD_STEP_ID[] = "GNProjectManager.BuildStep";
inline constexpr char GN_BUILD_DIR[] = "./out/%{Asciify:%{Kit:FileSystemName}-%{BuildConfig:Name}}";

inline constexpr char GN_BUILD_CONFIG_ID[] = "GNProjectManager.BuildConfiguration";
inline constexpr char OUTPUT_PREFIX[] = "[gn] ";

namespace SettingsPage {

inline constexpr char TOOLS_ID[] = "Z.GNProjectManager.SettingsPage.Tools";
inline constexpr char CATEGORY[] = "Z.GN";

} // namespace SettingsPage

namespace ToolsSettings {

inline constexpr char FILENAME[] = "gntools.xml";
inline constexpr char ENTRY_KEY[] = "GNTool.";
inline constexpr char ENTRY_COUNT[] = "GNTool.Count";
inline constexpr char DEFAULT_TOOL_KEY[] = "GNTools.Default";
inline constexpr char EXE_KEY[] = "exe";
inline constexpr char AUTO_DETECTED_KEY[] = "autodetected";
inline constexpr char NAME_KEY[] = "name";
inline constexpr char ID_KEY[] = "uuid";

} // namespace ToolsSettings

namespace Targets {

inline constexpr char all[] = "all";
inline constexpr char clean[] = "clean";

} // namespace Targets

} // namespace GNProjectManager::Constants
