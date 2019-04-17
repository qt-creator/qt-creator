%{Cpp:LicenseTemplate}\
@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %{GLOBAL_GUARD}
#define %{GLOBAL_GUARD}
@endif

@if '%{QtModule}' != 'none'
#include <QtCore/qglobal.h>
@else
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define Q_DECL_EXPORT __declspec(dllexport)
#  define Q_DECL_IMPORT __declspec(dllimport)
#else
#  define Q_DECL_EXPORT     __attribute__((visibility("default")))
#  define Q_DECL_IMPORT     __attribute__((visibility("default")))
#endif
@endif

#if defined(%{LibraryDefine})
#  define %{LibraryExport} Q_DECL_EXPORT
#else
#  define %{LibraryExport} Q_DECL_IMPORT
#endif
@if ! '%{Cpp:PragmaOnce}'

#endif // %{GLOBAL_GUARD}
@endif
