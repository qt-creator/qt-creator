/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
#include "mcusupportcmakemapper.h"
#include "utils/namevalueitem.h"

#include <utils/algorithm.h>

namespace {
static const QHash<QString, QString> &envVarToCMakeVarMapping()
{
    static const QHash<QString, QString> mapping = {
        {"EVK_MIMXRT1060_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"EVK_MIMXRT1064_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"EVK_MIMXRT595_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"EVK_MIMXRT1170_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"EVKB_IMXRT1050_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"STM32Cube_FW_F7_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"STM32Cube_FW_F4_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"STM32Cube_FW_L4_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"STM32Cube_FW_H7_SDK_PATH","QUL_BOARD_SDK_DIR"},
        {"ARMGCC_DIR", "QUL_TARGET_TOOLCHAIN_DIR"},
        {"IAR_ARM_COMPILER_DIR", "QUL_TARGET_TOOLCHAIN_DIR"},
        {"GHS_COMPILER_DIR", "QUL_TARGET_TOOLCHAIN_DIR"},
        {"GHS_ARM_COMPILER_DIR", "QUL_TARGET_TOOLCHAIN_DIR"},
        {"EVK_MIMXRT1170_FREERTOS_PATH","FREERTOS_DIR"},
        {"EVK_MIMXRT1170_FREERTOS_PATH","FREERTOS_DIR"},
        {"IMXRT1064_FREERTOS_DIR","FREERTOS_DIR"},
        {"IMXRT595_FREERTOS_DIR","FREERTOS_DIR"},
        {"STM32F7_FREERTOS_DIR", "FREERTOS_DIR"},
        {"eFlashLoad_PATH","eFlashLoad_PATH"},
        {"RenesasFlashProgrammer_PATH", "RenesasFlashProgrammer_PATH"},
        {"MCUXpressoIDE_PATH", "MCUXpressoIDE_PATH"},
        {"JLINK_PATH", "JLINK_PATH"},
        {"TVII_GRAPHICS_DRIVER_DIR", "TVII_GRAPHICS_DRIVER_DIR"},
        {"CYPRESS_AUTO_FLASH_UTILITY_DIR", "CYPRESS_AUTO_FLASH_UTILITY_DIR"},
        {"EK_RA6M3G_E2_PROJECT_PATH", "EK_RA6M3G_E2_PROJECT_PATH"},
        {"EK_RA6M3G_FSP_PATH", "EK_RA6M3G_FSP_PATH"},
    };
    return mapping;
}
} // namespace

QList<CMakeProjectManager::CMakeConfigItem> McuSupport::Internal::mapEnvVarsToQul2xCmakeVars(
    const Utils::EnvironmentItems &envVars)
{
    const auto &mapping = envVarToCMakeVarMapping();
    auto cmakeVars = Utils::transform(envVars, [mapping](const Utils::EnvironmentItem &envVar) {
        return CMakeProjectManager::CMakeConfigItem(mapping.value(envVar.name, "").toUtf8(), envVar.value.toUtf8());
    }).toList();

    return Utils::filtered(cmakeVars, [](const CMakeProjectManager::CMakeConfigItem &item) {
        return !item.key.isEmpty();
    });
}
