// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSharedMemory>
#include <QSystemSemaphore>

namespace QmlDesigner {

class SharedMemory
{
    friend class SharedMemoryLocker;
public:
    SharedMemory();
    SharedMemory(const QString &key);
    ~SharedMemory();

    void setKey(const QString &key);
    QString key() const;

    bool create(int size, QSharedMemory::AccessMode mode = QSharedMemory::ReadWrite);
    int size() const;

    bool attach(QSharedMemory::AccessMode mode = QSharedMemory::ReadWrite);
    bool isAttached() const;
    bool detach();

    void *data();
    const void* constData() const;
    const void *data() const;

    bool lock();
    bool unlock();

    QSharedMemory::SharedMemoryError error() const;
    QString errorString() const;

protected:
#ifdef Q_OS_UNIX
    bool initKeyInternal();
    void cleanHandleInternal();
    bool createInternal(QSharedMemory::AccessMode mode, size_t size);
    bool attachInternal(QSharedMemory::AccessMode mode);
    bool detachInternal();
    int handle();

    void setErrorString(const QString &function);
#endif
private:
#ifndef Q_OS_UNIX
    QSharedMemory m_sharedMemory;
#else
    void *m_memory;
    size_t m_size;
    QString m_key;
    QByteArray m_nativeKey;
    QSharedMemory::SharedMemoryError m_error;
    QString m_errorString;
    QSystemSemaphore m_systemSemaphore;
    bool m_lockedByMe;
    int m_fileHandle;
    bool m_createdByMe;
#endif
};

} // namespace QmlDesigner
