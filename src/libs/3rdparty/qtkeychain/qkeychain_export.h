#pragma once

#include <qglobal.h>

#if defined(QTKEYCHAIN_LIBRARY)
#  define QKEYCHAIN_EXPORT Q_DECL_EXPORT
#elif defined(QTKEYCHAIN_STATIC_LIBRARY)
#  define QKEYCHAIN_EXPORT
#else
#  define QKEYCHAIN_EXPORT Q_DECL_IMPORT
#endif
