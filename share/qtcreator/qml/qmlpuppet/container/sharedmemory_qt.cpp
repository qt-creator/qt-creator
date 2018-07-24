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
