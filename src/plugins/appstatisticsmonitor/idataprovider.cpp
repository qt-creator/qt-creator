// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "idataprovider.h"

#include <utils/expected.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>

#include <QByteArray>
#include <QRegularExpression>
#include <QString>

#ifdef Q_OS_LINUX
#include <unistd.h>
#endif

using namespace Utils;

namespace AppStatisticsMonitor::Internal {

IDataProvider::IDataProvider(qint64 pid, QObject *parent)
    : QObject(parent)
    , m_pid(pid)
{
    m_timer.setInterval(1000);
    connect(&m_timer, &QTimer::timeout, this, [this] { handleTimeout(); });
    m_timer.start();
}

void IDataProvider::handleTimeout()
{
    m_memoryConsumption.append(getMemoryConsumption());
    m_cpuConsumption.append(getCpuConsumption());
    emit newDataAvailable();
}

QList<double> IDataProvider::memoryConsumptionHistory() const
{
    return m_memoryConsumption;
}

QList<double> IDataProvider::cpuConsumptionHistory() const
{
    return m_cpuConsumption;
}

double IDataProvider::memoryConsumptionLast() const
{
    return m_memoryConsumption.isEmpty() ? 0 : m_memoryConsumption.last();
}

double IDataProvider::cpuConsumptionLast() const
{
    return m_cpuConsumption.isEmpty() ? 0 : m_cpuConsumption.last();
}

// ------------------------- LinuxDataProvider --------------------------------
#ifdef Q_OS_LINUX
class LinuxDataProvider : public IDataProvider
{
public:
    LinuxDataProvider(qint64 pid, QObject *parent = nullptr)
        : IDataProvider(pid, parent)
    {}

    double getMemoryConsumption()
    {
        const FilePath statusMemory = FilePath::fromString(
            QStringLiteral("/proc/%1/status").arg(m_pid));
        const expected_str<QByteArray> statusMemoryContent = statusMemory.fileContents();

        if (!statusMemoryContent)
            return 0;

        int vmPeak = 0;
        const static QRegularExpression numberRX(QLatin1String("[^0-9]+"));
        for (const QByteArray &element : statusMemoryContent.value().split('\n')) {
            if (element.startsWith("VmHWM")) {
                const QString p = QString::fromUtf8(element);
                vmPeak = p.split(numberRX, Qt::SkipEmptyParts)[0].toLong();
            }
        }

        const FilePath meminfoFile("/proc/meminfo");
        const expected_str<QByteArray> meminfoContent = meminfoFile.fileContents();
        if (!meminfoContent)
            return 0;

        const auto meminfo = meminfoContent.value().split('\n');
        if (meminfo.isEmpty())
            return 0;

        const auto parts = QString::fromUtf8(meminfo.front()).split(numberRX, Qt::SkipEmptyParts);
        if (parts.isEmpty())
            return 0;

        return double(vmPeak) / parts[0].toDouble() * 100;
    }

    // Provides the CPU usage from the last request
    double getCpuConsumption()
    {
        const FilePath status = FilePath::fromString(QStringLiteral("/proc/%1/stat").arg(m_pid));
        const FilePath uptimeFile = FilePath::fromString(QStringLiteral("/proc/uptime"));
        const double clkTck = static_cast<double>(sysconf(_SC_CLK_TCK));
        const expected_str<QByteArray> statusFileContent = status.fileContents();
        const expected_str<QByteArray> uptimeFileContent = uptimeFile.fileContents();

        if (!statusFileContent.has_value() || !uptimeFileContent.has_value() || clkTck == 0)
            return 0;

        const QList<QByteArray> processStatus = statusFileContent.value().split(' ');
        if (processStatus.isEmpty() || processStatus.size() < 22)
            return 0;

        const double uptime = uptimeFileContent.value().split(' ')[0].toDouble();

        const double utime = processStatus[13].toDouble() / clkTck;
        const double stime = processStatus[14].toDouble() / clkTck;
        const double cutime = processStatus[15].toDouble() / clkTck;
        const double cstime = processStatus[16].toDouble() / clkTck;
        const double starttime = processStatus[21].toDouble() / clkTck;

        // Calculate CPU usage for last request
        const double currentTotalTime = utime + stime + cutime + cstime;

        const double elapsed = uptime - starttime;
        const double clicks = (currentTotalTime - m_lastTotalTime) * clkTck;
        const double timeClicks = (elapsed - m_lastRequestStartTime) * clkTck;

        m_lastTotalTime = currentTotalTime;
        m_lastRequestStartTime = elapsed;

        return timeClicks > 0 ? 100 * (clicks / timeClicks) : 0;
    }

    // Provides the usage all over the process lifetime
    // Can be used in the future for the process lifetime statistics

    // double LinuxDataProvider::getCpuConsumption()
    // {
    //     const FilePath status = FilePath::fromString(
    //         QStringLiteral("/proc/%1/stat").arg(m_pid));
    //     const FilePath uptimeFile = FilePath::fromString(QStringLiteral("/proc/uptime"));
    //     const double clkTck = static_cast<double>(sysconf(_SC_CLK_TCK));

    //     if (!status.fileContents().has_value() || !uptimeFile.fileContents().has_value() || clkTck == 0)
    //         return 0;

    //     const QVector<QByteArray> processStatus = status.fileContents().value().split(' ').toVector();
    //     const double uptime = uptimeFile.fileContents().value().split(' ')[0].toDouble();

    //     const double utime = processStatus[13].toDouble() / clkTck;
    //     const double stime = processStatus[14].toDouble() / clkTck;
    //     const double cutime = processStatus[15].toDouble() / clkTck;
    //     const double cstime = processStatus[16].toDouble() / clkTck;
    //     const double starttime = processStatus[21].toDouble() / clkTck;

    //     const double elapsed = uptime - starttime;
    //     const double usage_sec = utime + stime + cutime + cstime;
    //     const double usage = 100 * usage_sec / elapsed;

    //     return usage;
    // }
};

#endif

// ------------------------- WindowsDataProvider --------------------------------

#ifdef Q_OS_WIN
class WindowsDataProvider : public IDataProvider
{
public:
    WindowsDataProvider(qint64 pid, QObject *parent = nullptr)
        : IDataProvider(pid, parent)
    {}

    double getMemoryConsumption() { return 0; }

    double getCpuConsumption() { return 0; }

#if 0
    double getMemoryConsumptionWindows(qint64 pid)
    {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (hProcess == NULL) {
            std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
            return 1;
        }

        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (!GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            std::cerr << "Failed to retrieve process memory information. Error code: " << GetLastError() << std::endl;
            CloseHandle(hProcess);
            return 1;
        }

        std::cout << "Process ID: " << pid << std::endl;
        std::cout << "Memory consumption: " << pmc.PrivateUsage << " bytes" << std::endl;

        CloseHandle(hProcess);
        return pmc.PrivateUsage;
    }
#endif
};
#endif

// ------------------------- MacDataProvider --------------------------------

#ifdef Q_OS_MACOS
class MacDataProvider : public IDataProvider
{
public:
    MacDataProvider(qint64 pid, QObject *parent = nullptr)
        : IDataProvider(pid, parent)
    {}

    double getMemoryConsumption() { return 0; }

    double getCpuConsumption() { return 0; }
};
#endif

IDataProvider *createDataProvider(qint64 pid)
{
#ifdef Q_OS_WIN
    return new WindowsDataProvider(pid);
#elif defined(Q_OS_MACOS)
    return new MacDataProvider(pid);
#else // Q_OS_LINUX
    return new LinuxDataProvider(pid);
#endif
}

} // namespace AppStatisticsMonitor::Internal
