%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GLOBAL_GUARD}
#define %{GLOBAL_GUARD}
@endif

#if defined(%{LibraryDefine})
#  define %{LibraryExport} Q_DECL_EXPORT
#else
#  define %{LibraryExport} Q_DECL_IMPORT
#endif
@if ! '%{Cpp:PragmaOnce}'

#endif // %{GLOBAL_GUARD}
@endif
