/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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

#pragma once

#include <QIODevice>
#include <QByteArray>

namespace qmt {

class QCompressedDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit QCompressedDevice(QObject *parent = nullptr);
    explicit QCompressedDevice(QIODevice *targetDevice, QObject *parent = nullptr);
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
    QIODevice *m_targetDevice = nullptr;
    QByteArray m_buffer;
    qint64 m_bytesInBuffer = 0;
    qint64 m_indexInBuffer = 0;
};

} // namespace qmt
