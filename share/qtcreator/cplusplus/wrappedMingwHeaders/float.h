/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

// MinGW's "public" float.h includes a "private" float.h via #include_next.
// We don't have access to the private header, because we cannot add its directory to
// the list of include paths due to other headers in there that confuse clang.
// Therefore, we prevent inclusion of the private header by setting a magic macro.
// See also QTCREATORBUG-24251.
#ifdef __MINGW32__ // Redundant, but let's play it safe.
#define __FLOAT_H
#include_next <float.h>
#endif
