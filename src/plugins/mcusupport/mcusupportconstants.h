// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace McuSupport::Internal::Constants {

inline constexpr char DEVICE_TYPE[]{"McuSupport.DeviceType"};
inline constexpr char DEVICE_ID[]{"McuSupport.Device"};
inline constexpr char RUNCONFIGURATION[]{"McuSupport.RunConfiguration"};
inline constexpr char SETTINGS_ID[]{"CC.McuSupport.Configuration"};

inline constexpr char KIT_MCUTARGET_VENDOR_KEY[]{"McuSupport.McuTargetVendor"};
inline constexpr char KIT_MCUTARGET_MODEL_KEY[]{"McuSupport.McuTargetModel"};
inline constexpr char KIT_MCUTARGET_SDKVERSION_KEY[]{"McuSupport.McuTargetSdkVersion"};
inline constexpr char KIT_MCUTARGET_KITVERSION_KEY[]{"McuSupport.McuTargetKitVersion"};
inline constexpr char KIT_MCUTARGET_COLORDEPTH_KEY[]{"McuSupport.McuTargetColorDepth"};
inline constexpr char KIT_MCUTARGET_OS_KEY[]{"McuSupport.McuTargetOs"};
inline constexpr char KIT_MCUTARGET_TOOLCHAIN_KEY[]{"McuSupport.McuTargetToolchain"};

inline constexpr char SETTINGS_GROUP[]{"McuSupport"};
inline constexpr char SETTINGS_KEY_PACKAGE_PREFIX[]{"Package_"};
inline constexpr char SETTINGS_KEY_PACKAGE_QT_FOR_MCUS_SDK[]{"QtForMCUsSdk"}; // Key known by SDK installer
inline constexpr char SETTINGS_KEY_AUTOMATIC_KIT_CREATION[]{"AutomaticKitCreation"};
// Used to decide which platform will be displayed first in the MCU Options page
inline constexpr char SETTINGS_KEY_INITIAL_PLATFORM_KEY[]{"McuSupport.InitialPlatform"};

} // namespace McuSupport::Internal::Constants
