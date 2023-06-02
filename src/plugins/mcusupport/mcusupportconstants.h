// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace McuSupport::Internal::Constants {

const char DEVICE_TYPE[]{"McuSupport.DeviceType"};
const char DEVICE_ID[]{"McuSupport.Device"};
const char RUNCONFIGURATION[]{"McuSupport.RunConfiguration"};
const char SETTINGS_ID[]{"CC.McuSupport.Configuration"};

const char KIT_MCUTARGET_VENDOR_KEY[]{"McuSupport.McuTargetVendor"};
const char KIT_MCUTARGET_MODEL_KEY[]{"McuSupport.McuTargetModel"};
const char KIT_MCUTARGET_SDKVERSION_KEY[]{"McuSupport.McuTargetSdkVersion"};
const char KIT_MCUTARGET_KITVERSION_KEY[]{"McuSupport.McuTargetKitVersion"};
const char KIT_MCUTARGET_COLORDEPTH_KEY[]{"McuSupport.McuTargetColorDepth"};
const char KIT_MCUTARGET_OS_KEY[]{"McuSupport.McuTargetOs"};
const char KIT_MCUTARGET_TOOLCHAIN_KEY[]{"McuSupport.McuTargetToolchain"};

const char SETTINGS_GROUP[]{"McuSupport"};
const char SETTINGS_KEY_PACKAGE_PREFIX[]{"Package_"};
const char SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK[]{"QtForMCUsSdk"}; // Key known by SDK installer
const char SETTINGS_KEY_AUTOMATIC_KIT_CREATION[]{"AutomaticKitCreation"};
// Used to decide which platform will be displayed first in the MCU Options page
const char SETTINGS_KEY_INITIAL_PLATFORM_KEY[]{"McuSupport.InitialPlatform"};

} // namespace McuSupport::Internal::Constants
