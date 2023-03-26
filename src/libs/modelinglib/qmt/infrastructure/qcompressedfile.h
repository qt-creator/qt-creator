// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
