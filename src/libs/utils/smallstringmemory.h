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

#include <qsystemdetection.h>

#include <cstdlib>
#include <cstring>
#include <memory>

namespace Utils {

namespace Memory {

inline char *allocate(std::size_t size)
{
#ifdef Q_OS_WIN32
    return static_cast<char*>(_aligned_malloc(size, 64));
#else
    return static_cast<char*>(std::malloc(size));
#endif
}

inline void deallocate(char *memory)
{
#ifdef Q_OS_WIN32
    _aligned_free(memory);
#else
#pragma GCC diagnostic push
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
    std::free(memory);
#pragma GCC diagnostic pop
#endif
}

inline char *reallocate(char *oldMemory, std::size_t newSize)
{
#ifdef Q_OS_WIN32
    return static_cast<char*>(_aligned_realloc(oldMemory, newSize, 64));
#else
    return static_cast<char*>(std::realloc(oldMemory, newSize));
#endif
}

} // namespace Memory

} // namespace Utils
