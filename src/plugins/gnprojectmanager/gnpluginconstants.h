// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace GNProjectManager {
namespace Constants {

namespace Project {

const char MIMETYPE[] = "text/x-gn";
} // namespace Project

namespace BuildConfiguration {

const char PARAMETERS_KEY[] = "GNProjectManager.BuildConfig.Parameters";

}

const char GN_PROJECT_ID[] = "GNProjectManager.GNProject";
const char PROJECT_JSON[] = "project.json";
const char PROJECT_AGS[] = "args.gn";

const char GN_BUILD_STEP_ID[] = "GNProjectManager.BuildStep";
const char GN_BUILD_DIR[] = "./out/%{Asciify:%{Kit:FileSystemName}-%{BuildConfig:Name}}";

const char GN_BUILD_CONFIG_ID[] = "GNProjectManager.BuildConfiguration";
const char OUTPUT_PREFIX[] = "[gn] ";

namespace SettingsPage {

const char TOOLS_ID[] = "Z.GNProjectManager.SettingsPage.Tools";
const char CATEGORY[] = "Z.GN";

} // namespace SettingsPage

namespace ToolsSettings {

const char FILENAME[] = "gntools.xml";
const char ENTRY_KEY[] = "Tool.";
const char ENTRY_COUNT[] = "Tools.Count";
const char EXE_KEY[] = "exe";
const char AUTO_DETECTED_KEY[] = "autodetected";
const char NAME_KEY[] = "name";
const char ID_KEY[] = "uuid";

} // namespace ToolsSettings

namespace Targets {

const char all[] = "all";
const char clean[] = "clean";

} // namespace Targets

} // namespace Constants
} // namespace GNProjectManager
