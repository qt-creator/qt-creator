@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %ProjectName:h%_CONSTANTS_H
#define %ProjectName:h%_CONSTANTS_H
@endif

namespace %PluginName% {
namespace Constants {

const char ACTION_ID[] = "%PluginName%.Action";
const char MENU_ID[] = "%PluginName%.Menu";

} // namespace %PluginName%
} // namespace Constants

@if ! '%{Cpp:PragmaOnce}'
#endif // %ProjectName:h%_CONSTANTS_H
@endif
