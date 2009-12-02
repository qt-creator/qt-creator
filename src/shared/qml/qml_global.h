#ifndef QML_GLOBAL_H
#define QML_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QML_BUILD_LIB)
#  define QML_EXPORT Q_DECL_EXPORT
#elif defined(QML_BUILD_STATIC_LIB)
#  define QML_EXPORT
#else
#  define QML_EXPORT Q_DECL_IMPORT
#endif

#endif // QML_GLOBAL_H
