// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// MinGW's "public" float.h includes a "private" float.h via #include_next.
// We don't have access to the private header, because we cannot add its directory to
// the list of include paths due to other headers in there that confuse clang.
// Therefore, we prevent inclusion of the private header by setting a magic macro.
// See also QTCREATORBUG-24251.
#ifdef __MINGW32__ // Redundant, but let's play it safe.
#define __FLOAT_H
#include_next <float.h>
#endif
