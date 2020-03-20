/****************************************************************************
**
** Copyright (C) 2018 Benjamin Balga
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

#include "serialterminalsettings.h"

#include <utils/outputformat.h>

#include <QObject>
#include <QSerialPort>
#include <QTimer>

namespace Utils { class OutputFormatter; }

namespace SerialTerminal {
namespace Internal {

// Handle serial port connect/disconnect/auto-reconnect, data read/write and info/error messages.
class SerialControl : public QObject
{
    Q_OBJECT
public:
    enum StopResult {
        StoppedSynchronously, // Stopped.
        AsynchronousStop      // Stop sequence has been started
    };

    explicit SerialControl(const Settings &settings, QObject *parent = nullptr);

    bool start();

    void stop(bool force = false);
    bool isRunning() const;

    QString displayName() const;

    bool canReUseOutputPane(const SerialControl *other) const;

    void appendMessage(const QString &msg, Utils::OutputFormat format);

    QString portName() const;
    void setPortName(const QString &name);

    qint32 baudRate() const;
    void setBaudRate(qint32 baudRate);
    QString baudRateText() const;

    void pulseDataTerminalReady();

    qint64 writeData(const QByteArray &data);

signals:
    void appendMessageRequested(SerialControl *serialControl,
                                const QString &msg, Utils::OutputFormat format);
    void started();
    void finished();
    void runningChanged(bool running);

private:
    void handleReadyRead();
    void reconnectTimeout();
    void handleError(QSerialPort::SerialPortError error);

protected:
    void tryReconnect();

private:
    QString m_portName;
    QSerialPort m_serialPort;
    QTimer m_reconnectTimer;

    bool m_initialDtrState = false;
    bool m_initialRtsState = false;
    bool m_clearInputOnSend = false;
    bool m_retrying = false;
    bool m_running = false;
};

} // namespace Internal
} // namespace SerialTerminal
