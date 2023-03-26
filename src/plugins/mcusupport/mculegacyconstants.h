// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace McuSupport::Internal::Legacy::Constants {

const char QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH[] = "bin/qmltocpp";
const char QUL_TOOLCHAIN_CMAKE_DIR[] = "lib/cmake/Qul/toolchain/";
const char QUL_ENV_VAR[] = "Qul_DIR";
const char QUL_CMAKE_VAR[] = "Qul_ROOT";
const char QUL_LABEL[] = "Qt for MCUs SDK";
const char BOARD_SDK_CMAKE_VAR[] = "QUL_BOARD_SDK_DIR";
const char SETTINGS_KEY_FREERTOS_PREFIX[]{"FreeRTOSSourcePackage_"};
const char TOOLCHAIN_DIR_CMAKE_VARIABLE[]{"QUL_TARGET_TOOLCHAIN_DIR"};
const char TOOLCHAIN_FILE_CMAKE_VARIABLE[]{"CMAKE_TOOLCHAIN_FILE"};

} // namespace McuSupport::Internal::Legacy::Constants
