// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"

#include <cstddef>
#include <new>

namespace CMakeLang {

// Nothing allocated from the pool is ever destroyed, so every AST node has to
// stay trivially destructible.
// Hand-rolled rather than a std::pmr::monotonic_buffer_resource: the libc++ the
// OpenHarmony SDK carries has no <memory_resource>.
class CMAKELANG_EXPORT MemoryPool
{
    MemoryPool(const MemoryPool &other) = delete;
    void operator=(const MemoryPool &other) = delete;

public:
    MemoryPool() = default;
    ~MemoryPool()
    {
        for (Block *block = _blocks; block;) {
            Block *next = block->next;
            ::operator delete(block);
            block = next;
        }
    }

    void *allocate(size_t size)
    {
        static const size_t alignment = alignof(std::max_align_t);
        size = (size + alignment - 1) & ~(alignment - 1);
        if (size > _free)
            return allocateFromNewBlock(size);
        std::byte *result = _next;
        _next += size;
        _free -= size;
        return result;
    }

private:
    enum { InitialSize = 8 * 1024 };

    struct Block
    {
        Block *next;
        alignas(std::max_align_t) std::byte data[1];
    };

    void *allocateFromNewBlock(size_t size)
    {
        size_t blockSize = InitialSize;
        while (blockSize < size)
            blockSize *= 2;
        Block *block = static_cast<Block *>(::operator new(sizeof(Block) + blockSize));
        block->next = _blocks;
        _blocks = block;
        _next = block->data + size;
        _free = blockSize - size;
        return block->data;
    }

    // The AST of a typical CMake file fits in here, so parsing it does not
    // reach the upstream allocator at all.
    alignas(std::max_align_t) std::byte _buffer[InitialSize];
    std::byte *_next = _buffer;
    size_t _free = sizeof(_buffer);
    Block *_blocks = nullptr;
};

class CMAKELANG_EXPORT Managed
{
    Managed(const Managed &other) = delete;
    void operator=(const Managed &other) = delete;

public:
    Managed() = default;

    void *operator new(size_t size, MemoryPool *pool) { return pool->allocate(size); }
    void operator delete(void *) {}
    void operator delete(void *, MemoryPool *) {}

protected:
    ~Managed() = default;
};

} // namespace CMakeLang
