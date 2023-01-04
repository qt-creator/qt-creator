// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sharedmemory.h"

namespace QmlDesigner {

SharedMemory::SharedMemory() = default;

SharedMemory::SharedMemory(const QString &key)
    : m_sharedMemory(key)
{
}

SharedMemory::~SharedMemory() = default;

void SharedMemory::setKey(const QString &key)
{
    m_sharedMemory.setKey(key);
}

QString SharedMemory::key() const
{
    return m_sharedMemory.key();
}

bool SharedMemory::create(int size, QSharedMemory::AccessMode mode)
{
    return m_sharedMemory.create(size, mode);
}

int SharedMemory::size() const
{
    return m_sharedMemory.size();
}

bool SharedMemory::attach(QSharedMemory::AccessMode mode)
{
    return m_sharedMemory.attach(mode);
}

bool SharedMemory::isAttached() const
{
    return m_sharedMemory.isAttached();
}

bool SharedMemory::detach()
{
    return m_sharedMemory.detach();
}

void *SharedMemory::data()
{
    return m_sharedMemory.data();
}

const void *SharedMemory::constData() const
{
    return m_sharedMemory.constData();
}

const void *SharedMemory::data() const
{
    return m_sharedMemory.data();
}

bool SharedMemory::lock()
{
    return m_sharedMemory.lock();
}

bool SharedMemory::unlock()
{
    return m_sharedMemory.unlock();
}

QSharedMemory::SharedMemoryError SharedMemory::error() const
{
    return m_sharedMemory.error();
}

QString SharedMemory::errorString() const
{
    return m_sharedMemory.errorString();
}

} // namespace QmlDesigner
