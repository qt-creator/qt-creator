// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "glslmemorypool.h"
#include <cstring>
#include <cassert>

using namespace GLSL;

MemoryPool::MemoryPool()
    : _blocks(nullptr),
      _allocatedBlocks(0),
      _blockCount(-1),
      _ptr(nullptr),
      _end(nullptr)
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
    _ptr = _end = nullptr;
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
            _blocks[index] = nullptr;
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

