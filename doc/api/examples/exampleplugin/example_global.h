#ifndef EXAMPLE_GLOBAL_H
#define EXAMPLE_GLOBAL_H

#include <QtCore/QtGlobal>

#if defined(EXAMPLE_LIBRARY)
#  define EXAMPLESHARED_EXPORT Q_DECL_EXPORT
#else
#  define EXAMPLESHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // EXAMPLE_GLOBAL_H

