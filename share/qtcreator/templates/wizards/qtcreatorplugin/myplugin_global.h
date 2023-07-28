%{Cpp:LicenseTemplate}\
#pragma once

#include <qglobal.h>

#if defined(%{LibraryDefine})
#  define %{LibraryExport} Q_DECL_EXPORT
#else
#  define %{LibraryExport} Q_DECL_IMPORT
#endif
