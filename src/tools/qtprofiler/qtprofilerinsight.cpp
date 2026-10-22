// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtprofilerinsight.h"

#include "qtprofilersettings.h"

#include <profiler/profilerusage.h>

#include <app/app_version.h>

#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>

#ifdef QTPROFILER_WITH_INSIGHT
#include <QInsightConfiguration>
#include <QInsightTracker>
#include <QSysInfo>

#include <memory>
#endif

using namespace Profiler::Internal;

namespace QtProfiler {

#ifdef QTPROFILER_WITH_INSIGHT

static std::unique_ptr<QInsightTracker> &tracker()
{
    static std::unique_ptr<QInsightTracker> theTracker;
    return theTracker;
}

static void report(const QString &key, const QString &data)
{
    if (!tracker() || !tracker()->isEnabled())
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 2)
    tracker()->contextData(key, data);
#else
    tracker()->interaction(key, data, 0);
#endif
}

static void startTracker()
{
    if (tracker()) {
        tracker()->setEnabled(true);
        return;
    }
    tracker().reset(new QInsightTracker);
    QInsightConfiguration *config = tracker()->configuration();
    config->setEvents({});
    config->setSyncInterval(3600); // Seconds. Zero would keep the events on disk.
    config->setBatchSize(100);
    config->setStoragePath(
        QStandardPaths::writableLocation(QStandardPaths::CacheLocation) + "/insight");
    config->setDeviceModel(
        QString("%1 (%2)").arg(QSysInfo::productType(), QSysInfo::currentCpuArchitecture()));
    config->setDeviceVariant(QSysInfo::productVersion());
    config->setDeviceScreenType("NON_TOUCH");
    config->setPlatform("app");
    config->setAppBuild(Core::Constants::IDE_VERSION_DISPLAY);
    config->setServer(QTPROFILER_INSIGHT_URL);
    config->setToken(QTPROFILER_INSIGHT_TOKEN);
    tracker()->setEnabled(true);
    QObject::connect(ProfilerUsage::instance(), &ProfilerUsage::usageEvent, qApp, &report);
    QObject::connect(ProfilerUsage::instance(), &ProfilerUsage::usageTransition, qApp,
                     [](const QString &screen) {
        if (tracker() && tracker()->isEnabled())
            tracker()->transition(screen);
    });
    tracker()->startNewSession();
    // The screen the user is on already, such as when they agreed to this just now.
    if (const QString screen = ProfilerUsage::instance()->currentScreen(); !screen.isEmpty())
        tracker()->transition(screen);
}

bool usageStatisticsAvailable()
{
    return true;
}

void setUsageStatisticsEnabled(bool enabled)
{
    settings().usageStatistics.setValue(enabled);
    if (enabled)
        startTracker();
    else if (tracker())
        tracker()->setEnabled(false);
}

void shutdownUsageStatistics()
{
    tracker().reset();
}

#else // QTPROFILER_WITH_INSIGHT

bool usageStatisticsAvailable()
{
    return false;
}

void setUsageStatisticsEnabled(bool) {}

void shutdownUsageStatistics() {}

#endif // QTPROFILER_WITH_INSIGHT

void initUsageStatistics(QWidget *dialogParent)
{
    if (!usageStatisticsAvailable())
        return;

    if (!settings().usageStatisticsAsked()) {
        settings().usageStatisticsAsked.setValue(true);
        const auto answer = QMessageBox::question(
            dialogParent,
            QApplication::translate("QtProfiler", "Usage Statistics"),
            QApplication::translate(
                "QtProfiler",
                "%1 can collect pseudonymous information about your system and the way you use "
                "the application. The data is associated with a pseudonymous user ID generated "
                "only for this purpose. It does not contain the traces you record or open, "
                "paths, or the programs you profile, and The Qt Company will use it strictly "
                "for improving its products.\n\n"
                "You can change this at any time with the \"Usage Statistics\" button in the "
                "toolbar.\n\n"
                "Would you like to help improve %1?")
                .arg(QApplication::applicationDisplayName()),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        settings().usageStatistics.setValue(answer == QMessageBox::Yes);
    }

    if (settings().usageStatistics())
        setUsageStatisticsEnabled(true);
}

} // namespace QtProfiler
