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

#include "serialcontrol.h"
#include "serialterminalconstants.h"

#include <utils/outputformatter.h>

namespace SerialTerminal {
namespace Internal {

SerialControl::SerialControl(const Settings &settings, QObject *parent) :
    QObject(parent)
{
    m_serialPort.setBaudRate(settings.baudRate);
    m_serialPort.setDataBits(settings.dataBits);
    m_serialPort.setParity(settings.parity);
    m_serialPort.setStopBits(settings.stopBits);
    m_serialPort.setFlowControl(settings.flowControl);

    if (!settings.portName.isEmpty())
        m_serialPort.setPortName(settings.portName);

    m_initialDtrState = settings.initialDtrState;
    m_initialRtsState = settings.initialRtsState;
    m_clearInputOnSend = settings.clearInputOnSend;

    m_reconnectTimer.setInterval(Constants::RECONNECT_DELAY);
    m_reconnectTimer.setSingleShot(true);

    connect(&m_serialPort, &QSerialPort::readyRead,
            this, &SerialControl::handleReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred,
            this, &SerialControl::handleError);
    connect(&m_reconnectTimer, &QTimer::timeout,
            this, &SerialControl::reconnectTimeout);
}

bool SerialControl::start()
{
    stop();

    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        if (!m_retrying) {
            appendMessage(tr("Unable to open port %1: %2.")
                          .arg(portName(), m_serialPort.errorString()),
                          Utils::ErrorMessageFormat);
        }
        return false;
    }

    m_serialPort.setDataTerminalReady(m_initialDtrState);
    m_serialPort.setRequestToSend(m_initialRtsState);

    if (m_retrying)
        appendMessage(tr("Session resumed.") + QString("\n\n"), Utils::NormalMessageFormat);
    else
        appendMessage(tr("Starting new session on %1...").arg(portName()) + "\n", Utils::NormalMessageFormat);

    m_retrying = false;

    m_running = true;
    emit started();
    emit runningChanged(true);
    return true;
}

void SerialControl::stop(bool force)
{
    if (force) {
        // Stop retries
        m_reconnectTimer.stop();
        m_retrying = false;
    }

    // Close if opened
    if (m_serialPort.isOpen())
        m_serialPort.close();

    // Print paused or finished message
    if (force || (m_running && !m_retrying)) {
        appendMessage(QString("\n")
                      + tr("Session finished on %1.").arg(portName())
                      + QString("\n\n"),
                      Utils::NormalMessageFormat);

        m_running = false;
        emit finished();
        emit runningChanged(false);
    } else if (m_running && m_retrying) {
        appendMessage(QString("\n")
                      + tr("Session paused...")
                      + QString("\n"),
                      Utils::NormalMessageFormat);
        m_running = false;
        // MAYBE: send paused() signals?
    }
}

bool SerialControl::isRunning() const
{
    // Considered "running" if "paused" (i.e. trying to reconnect)
    return m_running || m_retrying;
}

QString SerialControl::displayName() const
{
    return portName().isEmpty() ? tr("No Port") : portName();
}

bool SerialControl::canReUseOutputPane(const SerialControl *other) const
{
    return other->portName() == portName();
}

void SerialControl::appendMessage(const QString &msg, Utils::OutputFormat format)
{
    emit appendMessageRequested(this, msg, format);
}

QString SerialControl::portName() const
{
    return m_serialPort.portName();
}

void SerialControl::setPortName(const QString &name)
{
    if (m_serialPort.portName() == name)
        return;
    m_serialPort.setPortName(name);
}

qint32 SerialControl::baudRate() const
{
    return m_serialPort.baudRate();
}

void SerialControl::setBaudRate(qint32 baudRate)
{
    if (m_serialPort.baudRate() == baudRate)
        return;
    m_serialPort.setBaudRate(baudRate);
}

QString SerialControl::baudRateText() const
{
    return QString::number(baudRate());
}

void SerialControl::pulseDataTerminalReady()
{
    m_serialPort.setDataTerminalReady(!m_initialDtrState);
    QTimer::singleShot(Constants::RESET_DELAY, [&]() {
        m_serialPort.setDataTerminalReady(m_initialDtrState);
    });
}

qint64 SerialControl::writeData(const QByteArray& data)
{
    return m_serialPort.write(data);
}

void SerialControl::handleReadyRead()
{
    const QByteArray ba = m_serialPort.readAll();
    // For now, UTF8 should be safe for most use cases
    appendMessage(QString::fromUtf8(ba), Utils::StdOutFormat);
    // TODO: add config for string format conversion
}

void SerialControl::reconnectTimeout()
{
    // No port name set, stop reconnecting
    if (portName().isEmpty()) {
        m_retrying = false;
        return;
    }

    // Try to reconnect, restart timer if failed
    if (start())
        m_retrying = false;
    else
        m_reconnectTimer.start();
}

void SerialControl::handleError(QSerialPort::SerialPortError error)
{
    if (!isRunning()) // No auto reconnect if not running
        return;

    if (!m_retrying && error != QSerialPort::NoError)
        appendMessage(QString("\n")
                      + tr("Serial port error: %1 (%2)").arg(m_serialPort.errorString()).arg(error)
                      + QString("\n"),
                      Utils::ErrorMessageFormat);

    // Activate auto-reconnect on some resource errors
    // TODO: add auto-reconnect option to settings
    switch (error) {
    case QSerialPort::OpenError:
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::WriteError:
    case QSerialPort::ReadError:
    case QSerialPort::ResourceError:
    case QSerialPort::UnsupportedOperationError:
    case QSerialPort::UnknownError:
    case QSerialPort::TimeoutError:
    case QSerialPort::NotOpenError:
        // Enable auto-reconnect if needed
        if (!m_reconnectTimer.isActive() && !portName().isEmpty()) {
            m_retrying = true;
            m_reconnectTimer.start();
        }
        break;

    default:
        break;
    }
}

} // namespace Internal
} // namespace SerialTerminal
