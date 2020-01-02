%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{CONSTANTS_GUARD}
#define %{CONSTANTS_GUARD}
@endif

namespace %{PluginName} {
namespace Constants {

const char ACTION_ID[] = "%{PluginName}.Action";
const char MENU_ID[] = "%{PluginName}.Menu";

} // namespace Constants
} // namespace %{PluginName}

@if ! '%{Cpp:PragmaOnce}'
#endif // %{CONSTANTS_GUARD}
@endif
