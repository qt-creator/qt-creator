#include_next <qglobal.h>

#ifdef Q_DECL_IMPORT
#undef Q_DECL_IMPORT
#endif

#ifdef Q_DECL_EXPORT
#undef Q_DECL_EXPORT
#endif

#define Q_DECL_IMPORT
#define Q_DECL_EXPORT
