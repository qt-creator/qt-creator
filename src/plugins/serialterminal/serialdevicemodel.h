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
