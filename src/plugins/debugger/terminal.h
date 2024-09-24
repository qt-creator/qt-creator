// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QCoreApplication>
#include <QSocketNotifier>

namespace Debugger::Internal {

class Terminal : public QObject
{
    Q_OBJECT

public:
    Terminal(QObject *parent = nullptr);

    void setup();
    bool isUsable() const;

    QByteArray slaveDevice() const { return m_slaveName; }

    int write(const QByteArray &msg);
    bool sendInterrupt();

signals:
    void stdOutReady(const QString &);
    void stdErrReady(const QString &);
    void error(const QString &);

private:
    void onSlaveReaderActivated(int fd);

    bool m_isUsable = false;
    int m_masterFd = -1;
    QSocketNotifier *m_masterReader = nullptr;
    QByteArray m_slaveName;
};

} // namespace Debugger::Internal
