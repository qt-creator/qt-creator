// Copyright (C) 2018 Benjamin Balga
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
