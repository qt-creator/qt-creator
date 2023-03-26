// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace BareMetal {
namespace Constants {

const char BareMetalOsType[] = "BareMetalOsType";

const char ACTION_ID[] = "BareMetal.Action";
const char MENU_ID[] = "BareMetal.Menu";

const char DEBUG_SERVER_PROVIDERS_SETTINGS_ID[] = "EE.BareMetal.DebugServerProvidersOptions";

//FIXME: The values were wrong after c654677bf729369e, but we keep them for now.
const char BAREMETAL_CUSTOMRUNCONFIG_ID[] = "BareMetal"; // Sic!
const char BAREMETAL_RUNCONFIG_ID[] = "BareMetalCustom"; // Sic!

// GDB Debugger Server Provider Ids.
const char GDBSERVER_OPENOCD_PROVIDER_ID[] = "BareMetal.GdbServerProvider.OpenOcd";
const char GDBSERVER_JLINK_PROVIDER_ID[] = "BareMetal.GdbServerProvider.JLink";
const char GDBSERVER_GENERIC_PROVIDER_ID[] = "BareMetal.GdbServerProvider.Generic";
const char GDBSERVER_STLINK_UTIL_PROVIDER_ID[] = "BareMetal.GdbServerProvider.STLinkUtil";
const char GDBSERVER_EBLINK_PROVIDER_ID[] = "BareMetal.GdbServerProvider.EBlink";

// uVision Debugger Server Provider Ids.
const char UVSC_SIMULATOR_PROVIDER_ID[] = "BareMetal.UvscServerProvider.Simulator";
const char UVSC_STLINK_PROVIDER_ID[] = "BareMetal.UvscServerProvider.StLink";
const char UVSC_JLINK_PROVIDER_ID[] = "BareMetal.UvscServerProvider.JLink";

// Toolchain types.
const char IAREW_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Iar";
const char KEIL_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Keil";
const char SDCC_TOOLCHAIN_TYPEID[] = "BareMetal.ToolChain.Sdcc";

} // namespace BareMetal
} // namespace Constants
