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

#include "glsl.h"
#include <new>

namespace GLSL {

class MemoryPool;

class GLSL_EXPORT MemoryPool
{
    MemoryPool(const MemoryPool &other);
    void operator =(const MemoryPool &other);

public:
    MemoryPool();
    ~MemoryPool();

    void reset();

    inline void *allocate(size_t size)
    {
        size = (size + 7) & ~7;
        if (_ptr && (_ptr + size < _end)) {
            void *addr = _ptr;
            _ptr += size;
            return addr;
        }
        return allocate_helper(size);
    }

private:
    void *allocate_helper(size_t size);

private:
    char **_blocks;
    int _allocatedBlocks;
    int _blockCount;
    char *_ptr;
    char *_end;

    enum
    {
        BLOCK_SIZE = 8 * 1024,
        DEFAULT_BLOCK_COUNT = 8
    };
};

class GLSL_EXPORT Managed
{
    Managed(const Managed &other);
    void operator = (const Managed &other);

public:
    Managed();
    virtual ~Managed();

    void *operator new(size_t size, MemoryPool *pool);
    void operator delete(void *);
    void operator delete(void *, MemoryPool *);
};

} // namespace GLSL
