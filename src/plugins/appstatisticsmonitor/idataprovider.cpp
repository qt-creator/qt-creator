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

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
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
    {
        MEMORYSTATUSEX memoryStatus;
        memoryStatus.dwLength = sizeof(memoryStatus);
        GlobalMemoryStatusEx(&memoryStatus);

        m_totalMemory = memoryStatus.ullTotalPhys;
    }

    double getMemoryConsumption()
    {
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_pid);

        PROCESS_MEMORY_COUNTERS pmc;
        SIZE_T memoryUsed = 0;
        if (GetProcessMemoryInfo(process, &pmc, sizeof(pmc))) {
            memoryUsed = pmc.WorkingSetSize;
            // Can be used in the future for the process lifetime statistics
            //double memoryUsedMB = static_cast<double>(memoryUsed) / (1024.0 * 1024.0);
        }

        CloseHandle(process);
        return static_cast<double>(memoryUsed) / static_cast<double>(m_totalMemory) * 100.0;
    }

    double getCpuConsumption()
    {
        ULARGE_INTEGER sysKernel, sysUser, procKernel, procUser;
        HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, m_pid);

        FILETIME creationTime, exitTime, kernelTime, userTime;
        GetProcessTimes(process, &creationTime, &exitTime, &kernelTime, &userTime);
        procKernel.LowPart = kernelTime.dwLowDateTime;
        procKernel.HighPart = kernelTime.dwHighDateTime;
        procUser.LowPart = userTime.dwLowDateTime;
        procUser.HighPart = userTime.dwHighDateTime;

        SYSTEMTIME sysTime;
        GetSystemTime(&sysTime);
        SystemTimeToFileTime(&sysTime, &kernelTime);
        SystemTimeToFileTime(&sysTime, &userTime);
        sysKernel.LowPart = kernelTime.dwLowDateTime;
        sysKernel.HighPart = kernelTime.dwHighDateTime;
        sysUser.LowPart = userTime.dwLowDateTime;
        sysUser.HighPart = userTime.dwHighDateTime;

        const double sysElapsedTime = sysKernel.QuadPart + sysUser.QuadPart
                                      - m_lastSysKernel.QuadPart - m_lastSysUser.QuadPart;
        const double procElapsedTime = procKernel.QuadPart + procUser.QuadPart
                                       - m_lastProcKernel.QuadPart - m_lastProcUser.QuadPart;
        const double cpuUsagePercent = (procElapsedTime / sysElapsedTime) * 100.0;

        m_lastProcKernel = procKernel;
        m_lastProcUser = procUser;
        m_lastSysKernel = sysKernel;
        m_lastSysUser = sysUser;

        CloseHandle(process);
        return cpuUsagePercent;
    }

private:
    ULARGE_INTEGER m_lastSysKernel = {{0, 0}}, m_lastSysUser = {{0, 0}},
                   m_lastProcKernel = {{0, 0}}, m_lastProcUser = {{0, 0}};

    DWORDLONG m_totalMemory = 0;
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
