#pragma once

#include <QtGlobal>

#if defined(%PluginName:u%_LIBRARY)
#  define %PluginName:u%SHARED_EXPORT Q_DECL_EXPORT
#else
#  define %PluginName:u%SHARED_EXPORT Q_DECL_IMPORT
#endif
