#pragma once

#include <QtGlobal>

#if defined(EXAMPLE_LIBRARY)
#  define EXAMPLESHARED_EXPORT Q_DECL_EXPORT
#else
#  define EXAMPLESHARED_EXPORT Q_DECL_IMPORT
#endif
