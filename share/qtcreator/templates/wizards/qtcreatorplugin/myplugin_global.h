#ifndef %PluginName:u%_GLOBAL_%CppHeaderSuffix:u%
#define %PluginName:u%_GLOBAL_%CppHeaderSuffix:u%

#include <QtGlobal>

#if defined(%PluginName:u%_LIBRARY)
#  define %PluginName:u%SHARED_EXPORT Q_DECL_EXPORT
#else
#  define %PluginName:u%SHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // %PluginName:u%_GLOBAL_H
