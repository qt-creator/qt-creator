// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace McuSupport::Internal::Legacy::Constants {

inline constexpr char QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH[] = "bin/qmltocpp";
inline constexpr char QUL_TOOLCHAIN_CMAKE_DIR[] = "lib/cmake/Qul/toolchain/";
inline constexpr char QUL_ENV_VAR[] = "Qul_DIR";
inline constexpr char QUL_CMAKE_VAR[] = "Qul_ROOT";
inline constexpr char QUL_LABEL[] = "Qt for MCUs SDK";
inline constexpr char BOARD_SDK_CMAKE_VAR[] = "QUL_BOARD_SDK_DIR";
inline constexpr char SETTINGS_KEY_FREERTOS_PREFIX[]{"FreeRTOSSourcePackage_"};
inline constexpr char TOOLCHAIN_DIR_CMAKE_VARIABLE[]{"QUL_TARGET_TOOLCHAIN_DIR"};
inline constexpr char TOOLCHAIN_FILE_CMAKE_VARIABLE[]{"CMAKE_TOOLCHAIN_FILE"};

} // namespace McuSupport::Internal::Legacy::Constants
