// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmakelang.h"

#include <cstddef>
#include <memory_resource>
#include <new>

namespace CMakeLang {

// Nothing allocated from the pool is ever destroyed, so every AST node has to
// stay trivially destructible.
class CMAKELANG_EXPORT MemoryPool
{
    MemoryPool(const MemoryPool &other) = delete;
    void operator=(const MemoryPool &other) = delete;

public:
    MemoryPool() = default;
    ~MemoryPool() = default;

    void *allocate(size_t size) { return _resource.allocate(size); }

private:
    enum { InitialSize = 8 * 1024 };

    // The AST of a typical CMake file fits in here, so parsing it does not
    // reach the upstream allocator at all.
    alignas(std::max_align_t) std::byte _buffer[InitialSize];
    std::pmr::monotonic_buffer_resource _resource{_buffer, sizeof(_buffer)};
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
