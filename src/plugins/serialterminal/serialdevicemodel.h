// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QAbstractListModel>
#include <QSerialPortInfo>
#include <QSet>

namespace SerialTerminal {
namespace Internal {

class SerialDeviceModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit SerialDeviceModel(QObject *parent = nullptr);

    QString portName(int index) const;

    QStringList baudRates() const;
    qint32 baudRate(int index) const;
    int indexForBaudRate(qint32 baudRate) const;

    void disablePort(const QString &portName);
    void enablePort(const QString &portName);
    int indexForPort(const QString &portName) const;

    void update();

    Qt::ItemFlags flags(const QModelIndex &index) const final;
    int rowCount(const QModelIndex &parent) const final;
    QVariant data(const QModelIndex &index, int role) const final;

private:
    QList<QSerialPortInfo> m_ports;
    QSet<QString> m_disabledPorts;
    QList<qint32> m_baudRates;
};

} // namespace Internal
} // namespace SerialTerminal
