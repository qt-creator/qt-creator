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

#include "qtlockedfile.h"

namespace SharedTools {

/*!
    \class QtLockedFile

    \brief The QtLockedFile class extends QFile with advisory locking functions.

    A file may be locked in read or write mode. Multiple instances of
    \e QtLockedFile, created in multiple processes running on the same
    machine, may have a file locked in read mode. Exactly one instance
    may have it locked in write mode. A read and a write lock cannot
    exist simultaneously on the same file.

    The file locks are advisory. This means that nothing prevents
    another process from manipulating a locked file using QFile or
    file system functions offered by the OS. Serialization is only
    guaranteed if all processes that access the file use
    QtLockedFile. Also, while holding a lock on a file, a process
    must not open the same file again (through any API), or locks
    can be unexpectedly lost.

    The lock provided by an instance of \e QtLockedFile is released
    whenever the program terminates. This is true even when the
    program crashes and no destructors are called.
*/

/*! \enum QtLockedFile::LockMode

    This enum describes the available lock modes.

    \value ReadLock A read lock.
    \value WriteLock A write lock.
    \value NoLock Neither a read lock nor a write lock.
*/

/*!
    Constructs an unlocked \e QtLockedFile object. This constructor behaves in the same way
    as \e QFile::QFile().

    \sa QFile::QFile()
*/
QtLockedFile::QtLockedFile()
    : QFile()
{
#ifdef Q_OS_WIN
    m_semaphore_hnd = 0;
    m_mutex_hnd = 0;
#endif
    m_lock_mode = NoLock;
}

/*!
    Constructs an unlocked QtLockedFile object with file \a name. This constructor behaves in
    the same way as \e QFile::QFile(const QString&).

    \sa QFile::QFile()
*/
QtLockedFile::QtLockedFile(const QString &name)
    : QFile(name)
{
#ifdef Q_OS_WIN
    m_semaphore_hnd = 0;
    m_mutex_hnd = 0;
#endif
    m_lock_mode = NoLock;
}

/*!
    Returns \e true if this object has a in read or write lock;
    otherwise returns \e false.

    \sa lockMode()
*/
bool QtLockedFile::isLocked() const
{
    return m_lock_mode != NoLock;
}

/*!
    Returns the type of lock currently held by this object, or \e QtLockedFile::NoLock.

    \sa isLocked()
*/
QtLockedFile::LockMode QtLockedFile::lockMode() const
{
    return m_lock_mode;
}

/*!
    \fn bool QtLockedFile::lock(LockMode mode, bool block = true)

    Obtains a lock of type \a mode.

    If \a block is true, this
    function will block until the lock is acquired. If \a block is
    false, this function returns \e false immediately if the lock cannot
    be acquired.

    If this object already has a lock of type \a mode, this function returns \e true immediately. If this object has a lock of a different type than \a mode, the lock
    is first released and then a new lock is obtained.

    This function returns \e true if, after it executes, the file is locked by this object,
    and \e false otherwise.

    \sa unlock(), isLocked(), lockMode()
*/

/*!
    \fn bool QtLockedFile::unlock()

    Releases a lock.

    If the object has no lock, this function returns immediately.

    This function returns \e true if, after it executes, the file is not locked by
    this object, and \e false otherwise.

    \sa lock(), isLocked(), lockMode()
*/

/*!
    \fn QtLockedFile::~QtLockedFile()

    Destroys the \e QtLockedFile object. If any locks were held, they are released.
*/

} // namespace SharedTools
