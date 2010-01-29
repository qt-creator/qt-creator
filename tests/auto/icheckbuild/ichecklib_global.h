#ifndef ICHECKLIB_GLOBAL_H
#define ICHECKLIB_GLOBAL_H

#include <QtCore/qglobal.h>


#ifdef ICHECK_APP_BUILD
#  define ICHECKLIBSHARED_EXPORT
#else
#  if defined(ICHECKLIB_LIBRARY)
#    define ICHECKLIBSHARED_EXPORT Q_DECL_EXPORT
#  else
#    define ICHECKLIBSHARED_EXPORT Q_DECL_IMPORT
#  endif
#endif

#endif // ICHECKLIB_GLOBAL_H
