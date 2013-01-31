/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

RecursiveMemoryPool::RecursiveMemoryPool(MemoryPool *pool)
    : _pool(pool),
      _blockCount(pool->_blockCount),
      _ptr(pool->_ptr),
      _end(pool->_end)
{
}

RecursiveMemoryPool::~RecursiveMemoryPool()
{
    _pool->_blockCount = _blockCount;
    _pool->_ptr = _ptr;
    _pool->_end = _end;
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

