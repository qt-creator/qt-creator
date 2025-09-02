// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Vcpkg::Constants {

const char WEBSITE_URL[] = "https://vcpkg.io/";
const char ENVVAR_VCPKG_ROOT[] = "VCPKG_ROOT";
const char VCPKG_COMMAND[] = "vcpkg";
const char VCPKGMANIFEST_EDITOR_ID[] = "Vcpkg.VcpkgManifestEditor";
const char VCPKGMANIFEST_MIMETYPE[] = "application/vcpkg.manifest+json";

namespace Settings {
const char GENERAL_ID[] = "Vcpkg.VcpkgSettings";
const char GROUP_ID[] = "Vcpkg";
const char CATEGORY[] = "K.CMake"; // Yep, it is was pointed into K.CMake originally
const char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";
} // namespace Settings


} // namespace Vcpkg::Constants
