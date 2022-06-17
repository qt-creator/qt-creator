/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

namespace McuSupport::Internal::Legacy::Constants {

const char QT_FOR_MCUS_SDK_PACKAGE_VALIDATION_PATH[] = "bin/qmltocpp";
const char QUL_TOOLCHAIN_CMAKE_DIR[] = "lib/cmake/Qul/toolchain/";
const char QUL_ENV_VAR[] = "Qul_ROOT";
const char BOARD_SDK_CMAKE_VAR[] = "QUL_BOARD_SDK_DIR";
const char SETTINGS_KEY_FREERTOS_PREFIX[]{"FreeRTOSSourcePackage_"};
const char TOOLCHAIN_DIR_CMAKE_VARIABLE[]{"QUL_TARGET_TOOLCHAIN_DIR"};
const char TOOLCHAIN_FILE_CMAKE_VARIABLE[]{"CMAKE_TOOLCHAIN_FILE"};

} // namespace McuSupport::Internal::Legacy::Constants
