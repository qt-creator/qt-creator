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
#include <qdir.h>
#include <qcryptographichash.h>

#include "qplatformdefs.h"

#include <errno.h>

#include <sys/mman.h>
#include <sys/stat.h> /* For mode constants */
#include <fcntl.h> /* For O_* constants */

#include <sys/types.h>
#include <sys/stat.h>
#ifdef Q_OS_OSX
#include <sys/posix_shm.h>
#endif
#include <fcntl.h>
#include <unistd.h>

#include <private/qcore_unix_p.h>

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
#define QStringLiteral(str) QString::fromLatin1(str)
#endif

namespace QmlDesigner {

class SharedMemoryLocker
{
public:
    SharedMemoryLocker(SharedMemory *sharedMemory) : m_sharedMemory(sharedMemory)
    {
        Q_ASSERT(m_sharedMemory);
    }

    ~SharedMemoryLocker()
    {
        if (m_sharedMemory)
            m_sharedMemory->unlock();
    }

    bool tryLocker(const QString &function) {
        if (!m_sharedMemory)
            return false;

        if (m_sharedMemory->lock())
            return true;

        m_sharedMemory->m_errorString = QStringLiteral("%1: unable to lock").arg(function);
        m_sharedMemory->m_error = QSharedMemory::LockError;
        m_sharedMemory = 0;
        return false;
    }

private:
    SharedMemory *m_sharedMemory;
};

static QByteArray makePlatformSafeKey(const QString &key)
{
    if (key.isEmpty())
        return QByteArray();
    QByteArray data(QCryptographicHash::hash(key.toLatin1(), QCryptographicHash::Sha1).toBase64());

    data = data.replace('+', '-');
    data = data.replace('/', '_');

    data.truncate(31); // OS X is only supporting 31 byte long names

    return data;
}


SharedMemory::SharedMemory()
    : m_memory(0),
      m_size(0),
      m_error(QSharedMemory::NoError),
      m_systemSemaphore(QString()),
      m_lockedByMe(false),
      m_fileHandle(-1),
      m_createdByMe(false)
{
}

SharedMemory::SharedMemory(const QString &key)
    : m_memory(0),
      m_size(0),
      m_error(QSharedMemory::NoError),
      m_systemSemaphore(QString()),
      m_lockedByMe(false),
      m_fileHandle(-1),
      m_createdByMe(false)
{
    setKey(key);
}

SharedMemory::~SharedMemory()
{
    if (m_memory) {
        munmap(m_memory, m_size);
        m_memory = nullptr;
        m_size = 0;
    }

    if (m_fileHandle != -1) {
        close(m_fileHandle);
        cleanHandleInternal();
        if (m_createdByMe)
            shm_unlink(m_nativeKey);
    }

    setKey(QString());
}

void SharedMemory::setKey(const QString &key)
{
    if (key == m_key && makePlatformSafeKey(key) == m_nativeKey)
        return;

    if (isAttached())
        detach();

    m_key = key;
    m_nativeKey = makePlatformSafeKey(key);
}

QString SharedMemory::key() const
{
    return m_key;
}

bool SharedMemory::create(int size, QSharedMemory::AccessMode mode)
{
    if (!initKeyInternal())
        return false;


    m_systemSemaphore.setKey(m_key, 1, QSystemSemaphore::Create);


    QString function = QLatin1String("SharedMemory::create");

    SharedMemoryLocker lock(this);
    if (!m_key.isNull() && !lock.tryLocker(function))
        return false;

    if (size <= 0) {
        m_error = QSharedMemory::InvalidSize;
        m_errorString = QStringLiteral("%1: create size is less then 0").arg(function);
        return false;
    }

    return createInternal(mode ,size);
}

int SharedMemory::size() const
{
    return m_size;
}

bool SharedMemory::attach(QSharedMemory::AccessMode mode)
{
    if (isAttached() || !initKeyInternal())
        return false;

    SharedMemoryLocker lock(this);
    if (!m_key.isNull() && !lock.tryLocker(QStringLiteral("SharedMemory::attach")))
        return false;

    if (isAttached() || !handle())
        return false;

    return attachInternal(mode);
}


bool SharedMemory::isAttached() const
{
    return (0 != m_memory);
}

bool SharedMemory::detach()
{
    if (!isAttached())
        return false;

    SharedMemoryLocker lock(this);
    if (!m_key.isNull() && !lock.tryLocker(QStringLiteral("SharedMemory::detach")))
        return false;

    return detachInternal();
}

void *SharedMemory::data()
{
    return m_memory;
}

const void* SharedMemory::constData() const
{
    return m_memory;
}

const void *SharedMemory::data() const
{
    return m_memory;
}

bool SharedMemory::lock()
{
    if (m_lockedByMe) {
        qWarning("SharedMemory::lock: already locked");
        return true;
    }
    if (m_systemSemaphore.acquire()) {
        m_lockedByMe = true;
        return true;
    }
    QString function = QStringLiteral("SharedMemory::lock");
    m_errorString = QStringLiteral("%1: unable to lock").arg(function);
    m_error = QSharedMemory::LockError;
    return false;
}

bool SharedMemory::unlock()
{
    if (!m_lockedByMe)
        return false;
    m_lockedByMe = false;
    if (m_systemSemaphore.release())
        return true;
    QString function = QStringLiteral("SharedMemory::unlock");
    m_errorString = QStringLiteral("%1: unable to unlock").arg(function);
    m_error = QSharedMemory::LockError;
    return false;
}

QSharedMemory::SharedMemoryError SharedMemory::error() const
{
    return m_error;
}

QString SharedMemory::errorString() const
{
    return m_errorString;
}

void SharedMemory::setErrorString(const QString &function)
{
    // EINVAL is handled in functions so they can give better error strings
    switch (errno) {
    case EACCES:
        m_errorString = QStringLiteral("%1: permission denied").arg(function);
        m_error = QSharedMemory::PermissionDenied;
        break;
    case EEXIST:
        m_errorString = QStringLiteral("%1: already exists").arg(function);
        m_error = QSharedMemory::AlreadyExists;
        break;
    case ENOENT:
        m_errorString = QStringLiteral("%1: doesn't exist").arg(function);
        m_error = QSharedMemory::NotFound;
        break;
    case EMFILE:
    case ENOMEM:
    case ENOSPC:
        m_errorString = QStringLiteral("%1: out of resources").arg(function);
        m_error = QSharedMemory::OutOfResources;
        break;
    default:
        m_errorString = QStringLiteral("%1: unknown error %2")
                .arg(function).arg(QString::fromLocal8Bit(strerror(errno)));
        m_error = QSharedMemory::UnknownError;
    }
}

bool SharedMemory::initKeyInternal()
{
    cleanHandleInternal();

    m_systemSemaphore.setKey(QString(), 1);
    m_systemSemaphore.setKey(m_key, 1);
    if (m_systemSemaphore.error() != QSystemSemaphore::NoError) {
        m_errorString = QStringLiteral("SharedMemoryPrivate::initKey: unable to set key on lock");
        switch (m_systemSemaphore.error()) {
        case QSystemSemaphore::PermissionDenied:
            m_error = QSharedMemory::PermissionDenied;
            break;
        case QSystemSemaphore::KeyError:
            m_error = QSharedMemory::KeyError;
            break;
        case QSystemSemaphore::AlreadyExists:
            m_error = QSharedMemory::AlreadyExists;
            break;
        case QSystemSemaphore::NotFound:
            m_error = QSharedMemory::NotFound;
            break;
        case QSystemSemaphore::OutOfResources:
            m_error = QSharedMemory::OutOfResources;
            break;
        case QSystemSemaphore::UnknownError:
        default:
            m_error = QSharedMemory::UnknownError;
            break;
        }

        return false;
    }

    m_errorString.clear();
    m_error = QSharedMemory::NoError;
    return true;
}

int SharedMemory::handle()
{
    return m_fileHandle;
}

void SharedMemory::cleanHandleInternal()
{
    m_fileHandle = -1;
}

bool SharedMemory::createInternal(QSharedMemory::AccessMode mode, int size)
{
    detachInternal();

#ifdef Q_OS_OSX
    if (m_fileHandle > -1 && m_createdByMe) {
        close(m_fileHandle);
        shm_unlink(m_nativeKey.constData());
        m_fileHandle = -1;
    }
#endif

    if (m_fileHandle == -1) {
        int permission = mode == QSharedMemory::ReadOnly ? O_RDONLY : O_RDWR;
        m_fileHandle = shm_open(m_nativeKey.constData(), O_CREAT | permission, 0666);

        if (m_fileHandle == -1) {
            switch (errno) {
            case EINVAL:
                m_errorString = QStringLiteral("QSharedMemory::create: key is not invalid");
                m_error = QSharedMemory::KeyError;
                break;
            case EMFILE:
                m_errorString = QStringLiteral("QSharedMemory::create: maximum file limit reached");
                m_error = QSharedMemory::UnknownError;
                break;
            case ENAMETOOLONG:
                m_errorString = QStringLiteral("QSharedMemory::create: key is to long");
                m_error = QSharedMemory::KeyError;
                break;
            default:
                setErrorString(QStringLiteral("SharedMemory::create"));
            }
            return false;
        }

        m_createdByMe = true;
    }

    struct stat statBuffer;
    if (fstat(m_fileHandle, &statBuffer) == -1)
        return false;
    int fileSize = statBuffer.st_size;

    if (fileSize < size) {
        int returnValue = ftruncate(m_fileHandle, size);
        if (returnValue == -1) {
            switch (errno) {
            case EFBIG:
                m_errorString = QStringLiteral("QSharedMemory::create: size is to large");
                m_error = QSharedMemory::InvalidSize;
                break;
            default:
                setErrorString(QStringLiteral("SharedMemory::create"));
            }

            close(m_fileHandle);
            shm_unlink(m_nativeKey.constData());
            m_fileHandle = -1;
            m_size = 0;

            return false;
        }
    }

    int protection = mode == QSharedMemory::ReadOnly ? PROT_READ : PROT_WRITE;
    m_memory = mmap(0, size, protection, MAP_SHARED, m_fileHandle, 0);

    if (m_memory == MAP_FAILED) {
        close(m_fileHandle);
        shm_unlink(m_nativeKey.constData());
        m_memory = 0;
        m_fileHandle = -1;
        m_size = 0;

        return false;
    }

    m_size = size;

    return true;
}

bool SharedMemory::attachInternal(QSharedMemory::AccessMode mode)
{
    if (m_fileHandle == -1) {
        int permission = mode == QSharedMemory::ReadOnly ? O_RDONLY : O_RDWR;
        m_fileHandle = shm_open(m_nativeKey.constData(), permission, 0666);
        if (m_fileHandle == -1) {
            switch (errno) {
            case EINVAL:
                m_errorString = QStringLiteral("QSharedMemory::attach: key is invalid");
                m_error = QSharedMemory::KeyError;
                break;
            case EMFILE:
                m_errorString = QStringLiteral("QSharedMemory::attach: maximum file limit reached");
                m_error = QSharedMemory::UnknownError;
                break;
            case ENAMETOOLONG:
                m_errorString = QStringLiteral("QSharedMemory::attach: key is to long");
                m_error = QSharedMemory::KeyError;
                break;
            default:
                setErrorString(QStringLiteral("SharedMemory::attach"));
            }
            return false;
        }
    }

    struct stat statBuffer;
    if (fstat(m_fileHandle, &statBuffer) == -1)
        return false;
    int size = statBuffer.st_size;

    int protection = mode == QSharedMemory::ReadOnly ? PROT_READ : PROT_WRITE;
    m_memory = mmap(0, size, protection, MAP_SHARED, m_fileHandle, 0);

    if (m_memory == MAP_FAILED) {
        m_memory = 0;

        return false;
    }

    m_size = size;

    return true;
}

bool SharedMemory::detachInternal()
{
    if (m_memory) {
        munmap(m_memory, m_size);
        m_memory = 0;
        m_size = 0;
    }

    return false;
}

} // namespace QmlDesigner
