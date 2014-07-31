/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "sharedmemory.h"

namespace QmlDesigner {

SharedMemory::SharedMemory()
{
}

SharedMemory::SharedMemory(const QString &key)
    : m_sharedMemory(key)
{
}

SharedMemory::~SharedMemory()
{
}

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
