@if '%{Cpp:PragmaOnce}'
#pragma once
@else
#ifndef %ProjectName:h%_GLOBAL_H
#define %ProjectName:h%_GLOBAL_H
@endif

#include <QtGlobal>

#if defined(%PluginName:u%_LIBRARY)
#  define %PluginName:u%SHARED_EXPORT Q_DECL_EXPORT
#else
#  define %PluginName:u%SHARED_EXPORT Q_DECL_IMPORT
#endif

@if ! '%{Cpp:PragmaOnce}'
#endif // %ProjectName:h%_GLOBAL_H
@endif
