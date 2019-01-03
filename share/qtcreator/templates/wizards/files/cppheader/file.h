%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{JS: Cpp.headerGuard('%{FileName}')}
#define %{JS: Cpp.headerGuard('%{FileName}')}

#endif // %{JS: Cpp.headerGuard('%{FileName}')}
@endif
