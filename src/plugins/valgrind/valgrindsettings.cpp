/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "valgrindsettings.h"
#include "valgrindconfigwidget.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <valgrind/xmlprotocol/error.h>

#include <QSettings>
#include <QDebug>


using namespace Analyzer;

const char numCallersC[]  = "Analyzer.Valgrind.NumCallers";
const char trackOriginsC[] = "Analyzer.Valgrind.TrackOrigins";
const char suppressionFilesC[] = "Analyzer.Valgrind.SupressionFiles";
const char removedSuppressionFilesC[] = "Analyzer.Valgrind.RemovedSuppressionFiles";
const char addedSuppressionFilesC[] = "Analyzer.Valgrind.AddedSuppressionFiles";
const char filterExternalIssuesC[] = "Analyzer.Valgrind.FilterExternalIssues";
const char visibleErrorKindsC[] = "Analyzer.Valgrind.VisibleErrorKinds";

const char lastSuppressionDirectoryC[] = "Analyzer.Valgrind.LastSuppressionDirectory";
const char lastSuppressionHistoryC[] = "Analyzer.Valgrind.LastSuppressionHistory";

const char callgrindEnableCacheSimC[] = "Analyzer.Valgrind.Callgrind.EnableCacheSim";
const char callgrindEnableBranchSimC[] = "Analyzer.Valgrind.Callgrind.EnableBranchSim";
const char callgrindCollectSystimeC[] = "Analyzer.Valgrind.Callgrind.CollectSystime";
const char callgrindCollectBusEventsC[] = "Analyzer.Valgrind.Callgrind.CollectBusEvents";
const char callgrindEnableEventToolTipsC[] = "Analyzer.Valgrind.Callgrind.EnableEventToolTips";
const char callgrindMinimumCostRatioC[] = "Analyzer.Valgrind.Callgrind.MinimumCostRatio";
const char callgrindVisualisationMinimumCostRatioC[] = "Analyzer.Valgrind.Callgrind.VisualisationMinimumCostRatio";

const char callgrindCycleDetectionC[] = "Analyzer.Valgrind.Callgrind.CycleDetection";
const char callgrindShortenTemplates[] = "Analyzer.Valgrind.Callgrind.ShortenTemplates";
const char callgrindCostFormatC[] = "Analyzer.Valgrind.Callgrind.CostFormat";

const char valgrindExeC[] = "Analyzer.Valgrind.ValgrindExecutable";

namespace Valgrind {
namespace Internal {

//////////////////////////////////////////////////////////////////
//
// ValgrindBaseSettings
//
//////////////////////////////////////////////////////////////////

QVariantMap ValgrindBaseSettings::defaults() const
{
    QVariantMap map;

    // General
    map.insert(QLatin1String(valgrindExeC), QLatin1String("valgrind"));

    // Memcheck
    map.insert(QLatin1String(numCallersC), 25);
    map.insert(QLatin1String(trackOriginsC), true);
    map.insert(QLatin1String(filterExternalIssuesC), true);
    QVariantList defaultErrorKinds;
    for (int i = 0; i < Valgrind::XmlProtocol::MemcheckErrorKindCount; ++i)
        defaultErrorKinds << i;
    map.insert(QLatin1String(visibleErrorKindsC), defaultErrorKinds);

    // Callgrind
    map.insert(QLatin1String(callgrindEnableCacheSimC), false);
    map.insert(QLatin1String(callgrindEnableBranchSimC), false);
    map.insert(QLatin1String(callgrindCollectSystimeC), false);
    map.insert(QLatin1String(callgrindCollectBusEventsC), false);
    map.insert(QLatin1String(callgrindEnableEventToolTipsC), true);
    map.insert(QLatin1String(callgrindMinimumCostRatioC), 0.01);
    map.insert(QLatin1String(callgrindVisualisationMinimumCostRatioC), 10.0);

    return map;
}

void ValgrindBaseSettings::fromMap(const QVariantMap &map)
{
    // General
    setIfPresent(map, QLatin1String(valgrindExeC), &m_valgrindExecutable);

    // Memcheck
    setIfPresent(map, QLatin1String(numCallersC), &m_numCallers);
    setIfPresent(map, QLatin1String(trackOriginsC), &m_trackOrigins);
    setIfPresent(map, QLatin1String(filterExternalIssuesC), &m_filterExternalIssues);
    if (map.contains(QLatin1String(visibleErrorKindsC))) {
        m_visibleErrorKinds.clear();
        foreach (const QVariant &val, map.value(QLatin1String(visibleErrorKindsC)).toList())
            m_visibleErrorKinds << val.toInt();
    }

    // Callgrind
    setIfPresent(map, QLatin1String(callgrindEnableCacheSimC), &m_enableCacheSim);
    setIfPresent(map, QLatin1String(callgrindEnableBranchSimC), &m_enableBranchSim);
    setIfPresent(map, QLatin1String(callgrindCollectSystimeC), &m_collectSystime);
    setIfPresent(map, QLatin1String(callgrindCollectBusEventsC), &m_collectBusEvents);
    setIfPresent(map, QLatin1String(callgrindEnableEventToolTipsC), &m_enableEventToolTips);
    setIfPresent(map, QLatin1String(callgrindMinimumCostRatioC), &m_minimumInclusiveCostRatio);
    setIfPresent(map, QLatin1String(callgrindVisualisationMinimumCostRatioC),
                 &m_visualisationMinimumInclusiveCostRatio);

    emit changed();
}

QVariantMap ValgrindBaseSettings::toMap() const
{
    QVariantMap map;

    // General
    map.insert(QLatin1String(valgrindExeC), m_valgrindExecutable);

    // Memcheck
    map.insert(QLatin1String(numCallersC), m_numCallers);
    map.insert(QLatin1String(trackOriginsC), m_trackOrigins);
    map.insert(QLatin1String(filterExternalIssuesC), m_filterExternalIssues);
    QVariantList errorKinds;
    foreach (int i, m_visibleErrorKinds)
        errorKinds << i;
    map.insert(QLatin1String(visibleErrorKindsC), errorKinds);

    // Callgrind
    map.insert(QLatin1String(callgrindEnableCacheSimC), m_enableCacheSim);
    map.insert(QLatin1String(callgrindEnableBranchSimC), m_enableBranchSim);
    map.insert(QLatin1String(callgrindCollectSystimeC), m_collectSystime);
    map.insert(QLatin1String(callgrindCollectBusEventsC), m_collectBusEvents);
    map.insert(QLatin1String(callgrindEnableEventToolTipsC), m_enableEventToolTips);
    map.insert(QLatin1String(callgrindMinimumCostRatioC), m_minimumInclusiveCostRatio);
    map.insert(QLatin1String(callgrindVisualisationMinimumCostRatioC),
               m_visualisationMinimumInclusiveCostRatio);
    return map;
}

void ValgrindBaseSettings::setValgrindExecutable(const QString &valgrindExecutable)
{
    if (m_valgrindExecutable != valgrindExecutable) {
        m_valgrindExecutable = valgrindExecutable;
        emit valgrindExecutableChanged(valgrindExecutable);
    }
}

QString ValgrindBaseSettings::valgrindExecutable() const
{
    return m_valgrindExecutable;
}

Core::Id ValgrindBaseSettings::id() const
{
    return "Analyzer.Valgrind.Settings";
}

QString ValgrindBaseSettings::displayName() const
{
    return tr("Valgrind");
}

void ValgrindBaseSettings::setNumCallers(int numCallers)
{
    if (m_numCallers != numCallers) {
        m_numCallers = numCallers;
        emit numCallersChanged(numCallers);
    }
}

void ValgrindBaseSettings::setTrackOrigins(bool trackOrigins)
{
    if (m_trackOrigins != trackOrigins) {
        m_trackOrigins = trackOrigins;
        emit trackOriginsChanged(trackOrigins);
    }
}

void ValgrindBaseSettings::setFilterExternalIssues(bool filterExternalIssues)
{
    if (m_filterExternalIssues != filterExternalIssues) {
        m_filterExternalIssues = filterExternalIssues;
        emit filterExternalIssuesChanged(filterExternalIssues);
    }
}

void ValgrindBaseSettings::setVisibleErrorKinds(const QList<int> &visibleErrorKinds)
{
    if (m_visibleErrorKinds != visibleErrorKinds) {
        m_visibleErrorKinds = visibleErrorKinds;
        emit visibleErrorKindsChanged(visibleErrorKinds);
    }
}

void ValgrindBaseSettings::setEnableCacheSim(bool enable)
{
    if (m_enableCacheSim == enable)
        return;

    m_enableCacheSim = enable;
    emit enableCacheSimChanged(enable);
}

void ValgrindBaseSettings::setEnableBranchSim(bool enable)
{
    if (m_enableBranchSim == enable)
        return;

    m_enableBranchSim = enable;
    emit enableBranchSimChanged(enable);
}

void ValgrindBaseSettings::setCollectSystime(bool collect)
{
    if (m_collectSystime == collect)
        return;

    m_collectSystime = collect;
    emit collectSystimeChanged(collect);
}

void ValgrindBaseSettings::setCollectBusEvents(bool collect)
{
    if (m_collectBusEvents == collect)
        return;

    m_collectBusEvents = collect;
    emit collectBusEventsChanged(collect);
}

void ValgrindBaseSettings::setEnableEventToolTips(bool enable)
{
    if (m_enableEventToolTips == enable)
        return;

    m_enableEventToolTips = enable;
    emit enableEventToolTipsChanged(enable);
}

void ValgrindBaseSettings::setMinimumInclusiveCostRatio(
    double minimumInclusiveCostRatio)
{
    if (m_minimumInclusiveCostRatio == minimumInclusiveCostRatio)
        return;

    m_minimumInclusiveCostRatio = qBound(0.0, minimumInclusiveCostRatio, 100.0);
    emit minimumInclusiveCostRatioChanged(minimumInclusiveCostRatio);
}

void ValgrindBaseSettings::setVisualisationMinimumInclusiveCostRatio(
    double minimumInclusiveCostRatio)
{
    if (m_visualisationMinimumInclusiveCostRatio == minimumInclusiveCostRatio)
        return;

    m_visualisationMinimumInclusiveCostRatio = qBound(0.0, minimumInclusiveCostRatio, 100.0);
    emit visualisationMinimumInclusiveCostRatioChanged(minimumInclusiveCostRatio);
}


//////////////////////////////////////////////////////////////////
//
// ValgrindGlobalSettings
//
//////////////////////////////////////////////////////////////////

QWidget *ValgrindGlobalSettings::createConfigWidget(QWidget *parent)
{
    return new ValgrindConfigWidget(this, parent, true);
}

QVariantMap ValgrindGlobalSettings::defaults() const
{
    QVariantMap map = ValgrindBaseSettings::defaults();

    // Memcheck
    map.insert(QLatin1String(suppressionFilesC), QStringList());
    map.insert(QLatin1String(lastSuppressionDirectoryC), QString());
    map.insert(QLatin1String(lastSuppressionHistoryC), QStringList());

    // Callgrind
    map.insert(QLatin1String(callgrindCostFormatC), CostDelegate::FormatRelative);
    map.insert(QLatin1String(callgrindCycleDetectionC), true);

    return map;
}

void ValgrindGlobalSettings::fromMap(const QVariantMap &map)
{
    ValgrindBaseSettings::fromMap(map);

    // Memcheck
    m_suppressionFiles = map.value(QLatin1String(suppressionFilesC)).toStringList();
    m_lastSuppressionDirectory = map.value(QLatin1String(lastSuppressionDirectoryC)).toString();
    m_lastSuppressionHistory = map.value(QLatin1String(lastSuppressionHistoryC)).toStringList();

    // Callgrind
    // special code as the default one does not cope with the enum properly
    if (map.contains(QLatin1String(callgrindCostFormatC)))
        m_costFormat = static_cast<CostDelegate::CostFormat>(map.value(QLatin1String(callgrindCostFormatC)).toInt());
    setIfPresent(map, QLatin1String(callgrindCycleDetectionC), &m_detectCycles);
    setIfPresent(map, QLatin1String(callgrindShortenTemplates), &m_shortenTemplates);
}

AbstractAnalyzerSubConfig *ValgrindGlobalSettings::clone()
{
    ValgrindGlobalSettings *other = new ValgrindGlobalSettings;
    other->fromMap(toMap());
    return other;
}

QVariantMap ValgrindGlobalSettings::toMap() const
{
    QVariantMap map = ValgrindBaseSettings::toMap();

    // Memcheck
    map.insert(QLatin1String(suppressionFilesC), m_suppressionFiles);
    map.insert(QLatin1String(lastSuppressionDirectoryC), m_lastSuppressionDirectory);
    map.insert(QLatin1String(lastSuppressionHistoryC), m_lastSuppressionHistory);

    // Callgrind
    map.insert(QLatin1String(callgrindCostFormatC), m_costFormat);
    map.insert(QLatin1String(callgrindCycleDetectionC), m_detectCycles);
    map.insert(QLatin1String(callgrindShortenTemplates), m_shortenTemplates);

    return map;
}

//
// Memcheck
//
QStringList ValgrindGlobalSettings::suppressionFiles() const
{
    return m_suppressionFiles;
}

void ValgrindGlobalSettings::addSuppressionFiles(const QStringList &suppressions)
{
    foreach (const QString &s, suppressions)
        if (!m_suppressionFiles.contains(s))
            m_suppressionFiles.append(s);
}


void ValgrindGlobalSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    foreach (const QString &s, suppressions)
        m_suppressionFiles.removeAll(s);
}

QString ValgrindGlobalSettings::lastSuppressionDialogDirectory() const
{
    return m_lastSuppressionDirectory;
}

void ValgrindGlobalSettings::setLastSuppressionDialogDirectory(const QString &directory)
{
    m_lastSuppressionDirectory = directory;
}

QStringList ValgrindGlobalSettings::lastSuppressionDialogHistory() const
{
    return m_lastSuppressionHistory;
}

void ValgrindGlobalSettings::setLastSuppressionDialogHistory(const QStringList &history)
{
    m_lastSuppressionHistory = history;
}

//
// Callgrind
//
CostDelegate::CostFormat ValgrindGlobalSettings::costFormat() const
{
    return m_costFormat;
}

void ValgrindGlobalSettings::setCostFormat(CostDelegate::CostFormat format)
{
    m_costFormat = format;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

bool ValgrindGlobalSettings::detectCycles() const
{
    return m_detectCycles;
}

void ValgrindGlobalSettings::setDetectCycles(bool on)
{
    m_detectCycles = on;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

bool ValgrindGlobalSettings::shortenTemplates() const
{
    return m_shortenTemplates;
}

void ValgrindGlobalSettings::setShortenTemplates(bool on)
{
    m_shortenTemplates = on;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

ValgrindGlobalSettings *globalValgrindSettings()
{
    ValgrindGlobalSettings *ret = AnalyzerGlobalSettings::instance()->subConfig<ValgrindGlobalSettings>();
    QTC_ASSERT(ret, return 0);
    return ret;
}

//////////////////////////////////////////////////////////////////
//
// ValgrindProjectSettings
//
//////////////////////////////////////////////////////////////////

QWidget *ValgrindProjectSettings::createConfigWidget(QWidget *parent)
{
    return new ValgrindConfigWidget(this, parent, false);
}

QVariantMap ValgrindProjectSettings::defaults() const
{
    QVariantMap map = ValgrindBaseSettings::defaults();

    // Memcheck
    map.insert(QLatin1String(addedSuppressionFilesC), QStringList());
    map.insert(QLatin1String(removedSuppressionFilesC), QStringList());

    return map;
}

void ValgrindProjectSettings::fromMap(const QVariantMap &map)
{
    ValgrindBaseSettings::fromMap(map);

    // Memcheck
    setIfPresent(map, QLatin1String(addedSuppressionFilesC), &m_addedSuppressionFiles);
    setIfPresent(map, QLatin1String(removedSuppressionFilesC), &m_disabledGlobalSuppressionFiles);
}

AbstractAnalyzerSubConfig *ValgrindProjectSettings::clone()
{
    ValgrindProjectSettings *other = new ValgrindProjectSettings;
    other->fromMap(toMap());
    return other;
}

QVariantMap ValgrindProjectSettings::toMap() const
{
    QVariantMap map = ValgrindBaseSettings::toMap();

    // Memcheck
    map.insert(QLatin1String(addedSuppressionFilesC), m_addedSuppressionFiles);
    map.insert(QLatin1String(removedSuppressionFilesC), m_disabledGlobalSuppressionFiles);

    return map;
}

//
// Memcheck
//

void ValgrindProjectSettings::addSuppressionFiles(const QStringList &suppressions)
{
    QStringList globalSuppressions = globalValgrindSettings()->suppressionFiles();
    foreach (const QString &s, suppressions) {
        if (m_addedSuppressionFiles.contains(s))
            continue;
        m_disabledGlobalSuppressionFiles.removeAll(s);
        if (!globalSuppressions.contains(s))
            m_addedSuppressionFiles.append(s);
    }
}

void ValgrindProjectSettings::removeSuppressionFiles(const QStringList &suppressions)
{
    QStringList globalSuppressions = globalValgrindSettings()->suppressionFiles();
    foreach (const QString &s, suppressions) {
        m_addedSuppressionFiles.removeAll(s);
        if (globalSuppressions.contains(s))
            m_disabledGlobalSuppressionFiles.append(s);
    }
}

QStringList ValgrindProjectSettings::suppressionFiles() const
{
    QStringList ret = globalValgrindSettings()->suppressionFiles();
    foreach (const QString &s, m_disabledGlobalSuppressionFiles)
        ret.removeAll(s);
    ret.append(m_addedSuppressionFiles);
    return ret;
}

} // namespace Internal
} // namespace Valgrind
