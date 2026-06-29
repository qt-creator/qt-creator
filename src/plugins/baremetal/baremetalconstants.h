// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace BareMetal {
namespace Constants {

inline constexpr char BareMetalOsType[] = "BareMetalOsType";

inline constexpr char ACTION_ID[] = "BareMetal.Action";
inline constexpr char MENU_ID[] = "BareMetal.Menu";

inline constexpr char DEBUG_SERVER_PROVIDERS_SETTINGS_ID[] = "EE.BareMetal.DebugServerProvidersOptions";

//FIXME: The values were wrong after c654677bf729369e, but we keep them for now.
inline constexpr char BAREMETAL_CUSTOMRUNCONFIG_ID[] = "BareMetal"; // Sic!
inline constexpr char BAREMETAL_RUNCONFIG_ID[] = "BareMetalCustom"; // Sic!

// GDB Debugger Server Provider Ids.
inline constexpr char GDBSERVER_OPENOCD_PROVIDER_ID[] = "BareMetal.GdbServerProvider.OpenOcd";
inline constexpr char GDBSERVER_JLINK_PROVIDER_ID[] = "BareMetal.GdbServerProvider.JLink";
inline constexpr char GDBSERVER_GENERIC_PROVIDER_ID[] = "BareMetal.GdbServerProvider.Generic";
inline constexpr char GDBSERVER_STLINK_UTIL_PROVIDER_ID[] = "BareMetal.GdbServerProvider.STLinkUtil";
inline constexpr char GDBSERVER_EBLINK_PROVIDER_ID[] = "BareMetal.GdbServerProvider.EBlink";

// uVision Debugger Server Provider Ids.
inline constexpr char UVSC_SIMULATOR_PROVIDER_ID[] = "BareMetal.UvscServerProvider.Simulator";
inline constexpr char UVSC_STLINK_PROVIDER_ID[] = "BareMetal.UvscServerProvider.StLink";
inline constexpr char UVSC_JLINK_PROVIDER_ID[] = "BareMetal.UvscServerProvider.JLink";

// Toolchain types.
inline constexpr char IAREW_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Iar";
inline constexpr char KEIL_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Keil";
inline constexpr char SDCC_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Sdcc";

} // namespace BareMetal
} // namespace Constants
