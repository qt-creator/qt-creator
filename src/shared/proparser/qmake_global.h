/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include <qglobal.h>

#if defined(QMAKE_AS_LIBRARY)
#  if defined(QMAKE_LIBRARY)
#    define QMAKE_EXPORT Q_DECL_EXPORT
#  else
#    define QMAKE_EXPORT Q_DECL_IMPORT
#  endif
#else
#  define QMAKE_EXPORT
#endif

// Be fast even for debug builds
// MinGW GCC 4.5+ has a problem with always_inline putTok and putBlockLen
#if defined(__GNUC__) && !(defined(__MINGW32__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 5)
# define ALWAYS_INLINE inline __attribute__((always_inline))
#elif defined(_MSC_VER)
# define ALWAYS_INLINE __forceinline
#else
# define ALWAYS_INLINE inline
#endif

#ifdef PROEVALUATOR_FULL
#  define PROEVALUATOR_DEBUG
#endif
