// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qiopipe.h"

QIOPipe::QIOPipe(QObject *parent)
    : QObject(parent)
    , m_end1(std::make_unique<QPipeEndPoint>())
    , m_end2(std::make_unique<QPipeEndPoint>())
{
    m_end1->setRemoteEndPoint(m_end2.get());
    m_end2->setRemoteEndPoint(m_end1.get());
}

bool QIOPipe::open(QIODevice::OpenMode mode)
{
    if (!m_end1->open(mode))
        return false;
    switch (mode & QIODevice::ReadWrite) {
    case QIODevice::WriteOnly:
    case QIODevice::ReadOnly:
        return m_end2->open(mode ^ QIODevice::ReadWrite);
    default:
        return m_end2->open(mode);
    }
}

bool QPipeEndPoint::isSequential() const
{
    return true;
}

qint64 QPipeEndPoint::bytesAvailable() const
{
    return m_buffer.size() + QIODevice::bytesAvailable();
}

qint64 QPipeEndPoint::readData(char *data, qint64 maxlen)
{
    maxlen = qMin(maxlen, static_cast<qint64>(m_buffer.size()));
    if (maxlen <= 0)
        return 0;

    Q_ASSERT(maxlen > 0);
    Q_ASSERT(maxlen <= std::numeric_limits<int>::max());
    memcpy(data, m_buffer.data(), static_cast<size_t>(maxlen));
    m_buffer = m_buffer.mid(static_cast<int>(maxlen));
    return maxlen;
}

qint64 QPipeEndPoint::writeData(const char *data, qint64 len)
{
    if (!m_remoteEndPoint)
        return -1;

    if (len <= 0)
        return 0;

    QByteArray &buffer = m_remoteEndPoint->m_buffer;
    const qint64 prevLen = buffer.size();
    Q_ASSERT(prevLen >= 0);
    len = qMin(len, std::numeric_limits<int>::max() - prevLen);

    if (len == 0)
        return 0;

    Q_ASSERT(len > 0);
    Q_ASSERT(prevLen + len > 0);
    Q_ASSERT(prevLen + len <= std::numeric_limits<int>::max());

    buffer.resize(static_cast<int>(prevLen + len));
    memcpy(buffer.data() + prevLen, data, static_cast<size_t>(len));
    emit bytesWritten(len);
    emit m_remoteEndPoint->readyRead();
    return len;
}
