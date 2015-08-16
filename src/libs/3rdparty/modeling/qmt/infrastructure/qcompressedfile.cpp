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
      _target_device(0),
      _bytes_in_buffer(0),
      _index_in_buffer(0)
{
}

QCompressedDevice::QCompressedDevice(QIODevice *target_device, QObject *parent)
    : QIODevice(parent),
      _target_device(target_device),
      _bytes_in_buffer(0),
      _index_in_buffer(0)
{
}

QCompressedDevice::~QCompressedDevice()
{
    flush();
}

void QCompressedDevice::setTargetDevice(QIODevice *target_device)
{
    _target_device = target_device;
}

void QCompressedDevice::close()
{
    flush();
    QIODevice::close();
}

qint64 QCompressedDevice::readData(char *data, qint64 maxlen)
{
    QMT_CHECK(_target_device);
    QMT_CHECK(_target_device->isOpen());
    QMT_CHECK(_target_device->openMode() == QIODevice::ReadOnly);

    if (_bytes_in_buffer == 0) {
        QByteArray compressed_buffer;
        int compressed_len = 0;
        if (_target_device->read((char *) &compressed_len, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        compressed_buffer.resize(compressed_len);
        qint64 compressed_bytes = _target_device->read(compressed_buffer.data(), compressed_len);
        _buffer = qUncompress((const uchar *) compressed_buffer.data(), compressed_bytes);
        _bytes_in_buffer = _buffer.size();
        if (_bytes_in_buffer == 0) {
            return 0;
        }
        _index_in_buffer = 0;
    }
    qint64 n = std::min(maxlen, _bytes_in_buffer);
    memcpy(data, _buffer.data() + _index_in_buffer, n);
    _bytes_in_buffer -= n;
    _index_in_buffer += n;
    return n;
}

qint64 QCompressedDevice::writeData(const char *data, qint64 len)
{
    QMT_CHECK(_target_device);
    QMT_CHECK(_target_device->isOpen());
    QMT_CHECK(_target_device->openMode() == QIODevice::WriteOnly);

    _buffer.append(data, len);
    if (_buffer.size() > 1024*1024) {
        QByteArray compressed_buffer = qCompress(_buffer);
        int compressed_len = (int) compressed_buffer.size();
        if (_target_device->write((const char *) &compressed_len, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        if (_target_device->write(compressed_buffer.data(), compressed_len) != compressed_buffer.size()) {
            return -1;
        }
        _buffer.clear();
    }
    return len;
}

qint64 QCompressedDevice::flush()
{
    if (openMode() == QIODevice::WriteOnly && _buffer.size() > 0) {
        QMT_CHECK(_target_device->isOpen());
        QMT_CHECK(_target_device->openMode() == QIODevice::WriteOnly);
        QByteArray compressed_buffer = qCompress(_buffer);
        int compressed_len = (int) compressed_buffer.size();
        if (_target_device->write((const char *) &compressed_len, sizeof(int)) != sizeof(int)) {
            return -1;
        }
        return _target_device->write(compressed_buffer.data(), compressed_len);
    }
    return 0;
}

}
