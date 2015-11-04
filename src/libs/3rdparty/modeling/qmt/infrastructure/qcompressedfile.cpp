/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qcompressedfile.h"

#include "qmt/infrastructure/qmtassert.h"

// TODO create real compressed files that can be read by some archiver (gzip)

namespace qmt {

QCompressedDevice::QCompressedDevice(QObject *parent)
    : QIODevice(parent),
      m_targetDevice(0),
      m_bytesInBuffer(0),
      m_indexInBuffer(0)
{
}

QCompressedDevice::QCompressedDevice(QIODevice *targetDevice, QObject *parent)
    : QIODevice(parent),
      m_targetDevice(targetDevice),
      m_bytesInBuffer(0),
      m_indexInBuffer(0)
{
}

QCompressedDevice::~QCompressedDevice()
{
    flush();
}

void QCompressedDevice::setTargetDevice(QIODevice *targetDevice)
{
    m_targetDevice = targetDevice;
}

void QCompressedDevice::close()
{
    flush();
    QIODevice::close();
}

qint64 QCompressedDevice::readData(char *data, qint64 maxlen)
{
    QMT_CHECK(m_targetDevice);
    QMT_CHECK(m_targetDevice->isOpen());
    QMT_CHECK(m_targetDevice->openMode() == QIODevice::ReadOnly);

    if (m_bytesInBuffer == 0) {
        QByteArray compressedBuffer;
        int compressedLen = 0;
        if (m_targetDevice->read((char *) &compressedLen, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        compressedBuffer.resize(compressedLen);
        qint64 compressedBytes = m_targetDevice->read(compressedBuffer.data(), compressedLen);
        m_buffer = qUncompress((const uchar *) compressedBuffer.data(), compressedBytes);
        m_bytesInBuffer = m_buffer.size();
        if (m_bytesInBuffer == 0) {
            return 0;
        }
        m_indexInBuffer = 0;
    }
    qint64 n = std::min(maxlen, m_bytesInBuffer);
    memcpy(data, m_buffer.data() + m_indexInBuffer, n);
    m_bytesInBuffer -= n;
    m_indexInBuffer += n;
    return n;
}

qint64 QCompressedDevice::writeData(const char *data, qint64 len)
{
    QMT_CHECK(m_targetDevice);
    QMT_CHECK(m_targetDevice->isOpen());
    QMT_CHECK(m_targetDevice->openMode() == QIODevice::WriteOnly);

    m_buffer.append(data, len);
    if (m_buffer.size() > 1024*1024) {
        QByteArray compressedBuffer = qCompress(m_buffer);
        int compressedLen = (int) compressedBuffer.size();
        if (m_targetDevice->write((const char *) &compressedLen, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        if (m_targetDevice->write(compressedBuffer.data(), compressedLen) != compressedBuffer.size()) {
            return -1;
        }
        m_buffer.clear();
    }
    return len;
}

qint64 QCompressedDevice::flush()
{
    if (openMode() == QIODevice::WriteOnly && m_buffer.size() > 0) {
        QMT_CHECK(m_targetDevice->isOpen());
        QMT_CHECK(m_targetDevice->openMode() == QIODevice::WriteOnly);
        QByteArray compressedBuffer = qCompress(m_buffer);
        int compressedLen = (int) compressedBuffer.size();
        if (m_targetDevice->write((const char *) &compressedLen, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        return m_targetDevice->write(compressedBuffer.data(), compressedLen);
    }
    return 0;
}

}
