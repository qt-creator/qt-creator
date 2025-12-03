// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QIODevice>
#include <QPointer>

class QPipeEndPoint : public QIODevice
{
public:
    bool isSequential() const override;
    qint64 bytesAvailable() const override;

    void setRemoteEndPoint(QPipeEndPoint *other) { m_remoteEndPoint = other; }

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QByteArray m_buffer;
    QPointer<QPipeEndPoint> m_remoteEndPoint;
};

class QIOPipe : public QObject
{
public:
    QIOPipe(QObject *parent = nullptr);

    bool open(QIODevice::OpenMode mode);

    QIODevice *end1() const { return m_end1.get(); }
    QIODevice *takeEnd1() { return m_end1.release(); }

    QIODevice *end2() const { return m_end2.get(); }
    QIODevice *takeEnd2() { return m_end2.release(); }

private:
    std::unique_ptr<QPipeEndPoint> m_end1;
    std::unique_ptr<QPipeEndPoint> m_end2;
};
