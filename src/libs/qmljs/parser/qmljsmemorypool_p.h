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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qmljsglobal_p.h"

#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>
#include <QtCore/qdebug.h>

#include <cstring>

QT_QML_BEGIN_NAMESPACE

namespace QmlJS {

class Managed;

class QML_PARSER_EXPORT MemoryPool : public QSharedData
{
    MemoryPool(const MemoryPool &other);
    void operator =(const MemoryPool &other);

public:
    MemoryPool()
        : _blocks(0),
          _allocatedBlocks(0),
          _blockCount(-1),
          _ptr(0),
          _end(0)
    { }

    ~MemoryPool()
    {
        if (_blocks) {
            for (int i = 0; i < _allocatedBlocks; ++i) {
                if (char *b = _blocks[i])
                    free(b);
            }

            free(_blocks);
        }
    }

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

    void reset()
    {
        _blockCount = -1;
        _ptr = _end = 0;
    }

    template <typename Tp> Tp *New() { return new (this->allocate(sizeof(Tp))) Tp(); }

private:
    void *allocate_helper(size_t size)
    {
        Q_ASSERT(size < BLOCK_SIZE);

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
            block = (char *) malloc(BLOCK_SIZE);

        _ptr = block;
        _end = _ptr + BLOCK_SIZE;

        void *addr = _ptr;
        _ptr += size;
        return addr;
    }

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

class QML_PARSER_EXPORT Managed
{
    Managed(const Managed &other);
    void operator = (const Managed &other);

public:
    Managed() {}
    ~Managed() {}

    void *operator new(size_t size, MemoryPool *pool) { return pool->allocate(size); }
    void operator delete(void *) {}
    void operator delete(void *, MemoryPool *) {}
};

} // namespace QmlJS

QT_QML_END_NAMESPACE
