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

#include <QByteArray>
#include <QPair>
#include <QSerialPort>
#include <QString>
#include <QVector>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace SerialTerminal {
namespace Internal {

class Settings {
public:
    explicit Settings();

    bool edited = false;
    qint32 baudRate = 9600;
    QSerialPort::DataBits dataBits = QSerialPort::Data8;
    QSerialPort::Parity parity = QSerialPort::NoParity;
    QSerialPort::StopBits stopBits = QSerialPort::OneStop;
    QSerialPort::FlowControl flowControl = QSerialPort::NoFlowControl;

    QString portName;

    bool initialDtrState = false;
    bool initialRtsState = false;
    unsigned int defaultLineEndingIndex;
    QVector<QPair<QString, QByteArray>> lineEndings;

    bool clearInputOnSend = false;

    void save(QSettings *settings);
    void load(QSettings *settings);

    void setBaudRate(qint32 br);
    void setPortName(const QString &name);

    QByteArray defaultLineEnding() const;
    QString defaultLineEndingText() const;
    void setDefaultLineEndingIndex(unsigned int index);

private:
    void saveLineEndings(QSettings &settings);
    void loadLineEndings(QSettings &settings);
};

} // namespace Internal
} // namespace SerialTerminal
