/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

namespace McuSupport {
namespace Constants {

const char TOOLCHAIN_TYPEID[] = "McuSupport.ToolChain.ARM-GCC";
const char DEVICE_TYPE[] = "McuSupport.DeviceType";
const char DEVICE_ID[] = "McuSupport.Device";
const char MCUSUPPORT_QT_VERSION[] = "Qt4ProjectManager.QtVersion.McuSupport";
const char RUNCONFIGURATION[] = "McuSupport.RunConfiguration";
const char SETTINGS_ID[] = "CC.McuSupport.Configuration";
const char KIT_BOARD_VENDOR_KEY[] = "McuSupport.BoardVendor";
const char KIT_BOARD_MODEL_KEY[] = "McuSupport.BoardModel";

const char ENVVAR_ARMGCC_DIR[] = "ARMGCC_DIR";
const char ENVVAR_STM32CUBE_FW_F7_SDK_PATH[] = "STM32Cube_FW_F7_SDK_PATH";

const char SETTINGS_GROUP[] = "McuSupport";
const char SETTINGS_KEY_PACKAGE_PREFIX[] = "package_";
const char SETTINGS_KEY_PACKAGE_ARMGCC[] = "ArmGcc";
const char SETTINGS_KEY_STM32CUBE_FW_F7_SDK_PATH[] = "STM32Cube_FW_F7_SDK_PATH";
const char SETTINGS_KEY_STM32CUBE_PROGRAMMER_PATH[] = "STM32Cube_Cube_Programmer_Path";

} // namespace McuSupport
} // namespace Constants
