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

#include "glslmemorypool.h"
#include <cstring>
#include <cassert>

using namespace GLSL;

MemoryPool::MemoryPool()
    : _blocks(0),
      _allocatedBlocks(0),
      _blockCount(-1),
      _ptr(0),
      _end(0)
{ }

MemoryPool::~MemoryPool()
{
    if (_blocks) {
        for (int i = 0; i < _allocatedBlocks; ++i) {
            if (char *b = _blocks[i])
                std::free(b);
        }

        std::free(_blocks);
    }
}

void MemoryPool::reset()
{
    _blockCount = -1;
    _ptr = _end = 0;
}

void *MemoryPool::allocate_helper(size_t size)
{
    assert(size < BLOCK_SIZE);

    if (++_blockCount == _allocatedBlocks) {
        if (! _allocatedBlocks)
            _allocatedBlocks = DEFAULT_BLOCK_COUNT;
        else
            _allocatedBlocks *= 2;

        _blocks = (char **) realloc(_blocks, sizeof(char *) * _allocatedBlocks);

        for (int index = _blockCount; index < _allocatedBlocks; ++index)
            _blocks[index] = 0;
    }

    char *&block = _blocks[_blockCount];

    if (! block)
        block = (char *) std::malloc(BLOCK_SIZE);

    _ptr = block;
    _end = _ptr + BLOCK_SIZE;

    void *addr = _ptr;
    _ptr += size;
    return addr;
}

Managed::Managed()
{ }

Managed::~Managed()
{ }

void *Managed::operator new(size_t size, MemoryPool *pool)
{ return pool->allocate(size); }

void Managed::operator delete(void *)
{ }

void Managed::operator delete(void *, MemoryPool *)
{ }

