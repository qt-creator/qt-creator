// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <QList>
#include <QObject>
#include <QTimer>

namespace AppStatisticsMonitor::Internal {

class IDataProvider : public QObject
{
    Q_OBJECT
public:
    IDataProvider(qint64 pid, QObject *parent = nullptr);

    QList<double> memoryConsumptionHistory() const;
    QList<double> cpuConsumptionHistory() const;

    double memoryConsumptionLast() const;
    double cpuConsumptionLast() const;

protected:
    virtual double getMemoryConsumption() = 0;
    virtual double getCpuConsumption() = 0;

    QList<double> m_memoryConsumption;
    QList<double> m_cpuConsumption;
    qint64 m_pid;
    double m_lastTotalTime;
    double m_lastRequestStartTime;

signals:
    void newDataAvailable();

private:
    void handleTimeout();

    QTimer m_timer;
};

IDataProvider *createDataProvider(qint64 pid);

} // AppStatisticsMonitor::Internal
