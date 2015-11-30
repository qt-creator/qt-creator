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

#ifndef QCOMPRESSEDDEVICE_H
#define QCOMPRESSEDDEVICE_H

#include <QIODevice>
#include <QByteArray>

namespace qmt {

class QCompressedDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit QCompressedDevice(QObject *parent = 0);
    explicit QCompressedDevice(QIODevice *targetDevice, QObject *parent = 0);
    ~QCompressedDevice() override;

    QIODevice *targetDevice() const { return m_targetDevice; }
    void setTargetDevice(QIODevice *targetDevice);

    void close() override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

public:
    qint64 flush();

private:
    QIODevice *m_targetDevice = 0;
    QByteArray m_buffer;
    qint64 m_bytesInBuffer = 0;
    qint64 m_indexInBuffer = 0;
};

} // namespace qmt

#endif // QCOMPRESSEDDEVICE_H
