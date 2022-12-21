// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "serialdevicemodel.h"

#include <utils/algorithm.h>

namespace SerialTerminal {
namespace Internal {

SerialDeviceModel::SerialDeviceModel(QObject *parent) :
    QAbstractListModel(parent),
    m_baudRates(QSerialPortInfo::standardBaudRates())
{
}

QString SerialDeviceModel::portName(int index) const
{
    if (index < 0 || index >= m_ports.size())
        return QString();

    return m_ports.at(index).portName();
}

QStringList SerialDeviceModel::baudRates() const
{
    return Utils::transform(m_baudRates, [](int b) { return QString::number(b); });
}

qint32 SerialDeviceModel::baudRate(int index) const
{
    if (index < 0 || index >= m_baudRates.size())
        return 0;

    return m_baudRates.at(index);
}

int SerialDeviceModel::indexForBaudRate(qint32 baudRate) const
{
    return m_baudRates.indexOf(baudRate);
}

void SerialDeviceModel::disablePort(const QString &portName)
{
    m_disabledPorts.insert(portName);

    const int i = Utils::indexOf(m_ports, [&portName](const QSerialPortInfo &info) {
        return info.portName() == portName;
    });

    if (i >= 0) {
        const QModelIndex itemIndex = index(i);
        emit dataChanged(itemIndex, itemIndex, {Qt::DisplayRole});
    }
}

void SerialDeviceModel::enablePort(const QString &portName)
{
    m_disabledPorts.remove(portName);
}

int SerialDeviceModel::indexForPort(const QString &portName) const
{
    return Utils::indexOf(m_ports, [portName](const QSerialPortInfo &port) {
        return port.portName() == portName;
    });
}

void SerialDeviceModel::update()
{
    // Called from the combobox before popup, thus updated only when needed and immediately
    beginResetModel();
    m_ports.clear();
    const QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        const QString portName = serialPortInfo.portName();

        // TODO: add filter
        if (!portName.isEmpty())
            m_ports.append(serialPortInfo);
    }
    endResetModel();
}

Qt::ItemFlags SerialDeviceModel::flags(const QModelIndex &index) const
{
    auto f = QAbstractListModel::flags(index);
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ports.size())
        return f;

    if (m_disabledPorts.contains(m_ports.at(index.row()).portName()))
        f &= ~Qt::ItemIsEnabled;

    return f;
}

int SerialDeviceModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_ports.size();
}

QVariant SerialDeviceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    return m_ports.at(index.row()).portName();
}

} // namespace Internal
} // namespace SerialTerminal
