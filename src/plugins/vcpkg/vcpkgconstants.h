// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace Vcpkg::Constants {

inline constexpr char WEBSITE_URL[] = "https://vcpkg.io/";
inline constexpr char ENVVAR_VCPKG_ROOT[] = "VCPKG_ROOT";
inline constexpr char VCPKG_COMMAND[] = "vcpkg";
inline constexpr char VCPKGMANIFEST_EDITOR_ID[] = "Vcpkg.VcpkgManifestEditor";
inline constexpr char VCPKGMANIFEST_MIMETYPE[] = "application/vcpkg.manifest+json";

namespace Settings {
inline constexpr char GENERAL_ID[] = "Vcpkg.VcpkgSettings";
inline constexpr char GROUP_ID[] = "Vcpkg";
inline constexpr char CATEGORY[] = "K.CMake"; // Yep, it is was pointed into K.CMake originally
inline constexpr char USE_GLOBAL_SETTINGS[] = "UseGlobalSettings";
} // namespace Settings


} // namespace Vcpkg::Constants
