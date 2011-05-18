/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "callgrindsettings.h"

#include "callgrindconfigwidget.h"

#include <QtCore/QDebug>

using namespace Analyzer;

static const char callgrindEnableCacheSimC[] = "Analyzer.Valgrind.Callgrind.EnableCacheSim";
static const char callgrindEnableBranchSimC[] = "Analyzer.Valgrind.Callgrind.EnableBranchSim";
static const char callgrindCollectSystimeC[] = "Analyzer.Valgrind.Callgrind.CollectSystime";
static const char callgrindCollectBusEventsC[] = "Analyzer.Valgrind.Callgrind.CollectBusEvents";
static const char callgrindEnableEventToolTipsC[] = "Analyzer.Valgrind.Callgrind.EnableEventToolTips";
static const char callgrindMinimumCostRatioC[] = "Analyzer.Valgrind.Callgrind.MinimumCostRatio";
static const char callgrindVisualisationMinimumCostRatioC[] = "Analyzer.Valgrind.Callgrind.VisualisationMinimumCostRatio";

static const char callgrindCycleDetectionC[] = "Analyzer.Valgrind.Callgrind.CycleDetection";
static const char callgrindCostFormatC[] = "Analyzer.Valgrind.Callgrind.CostFormat";

namespace Callgrind {
namespace Internal {

void AbstractCallgrindSettings::setEnableCacheSim(bool enable)
{
    if (m_enableCacheSim == enable)
        return;

    m_enableCacheSim = enable;
    emit enableCacheSimChanged(enable);
}

void AbstractCallgrindSettings::setEnableBranchSim(bool enable)
{
    if (m_enableBranchSim == enable)
        return;

    m_enableBranchSim = enable;
    emit enableBranchSimChanged(enable);
}

void AbstractCallgrindSettings::setCollectSystime(bool collect)
{
    if (m_collectSystime == collect)
        return;

    m_collectSystime = collect;
    emit collectSystimeChanged(collect);
}

void AbstractCallgrindSettings::setCollectBusEvents(bool collect)
{
    if (m_collectBusEvents == collect)
        return;

    m_collectBusEvents = collect;
    emit collectBusEventsChanged(collect);
}

void AbstractCallgrindSettings::setEnableEventToolTips(bool enable)
{
    if (m_enableEventToolTips == enable)
        return;

    m_enableEventToolTips = enable;
    emit enableEventToolTipsChanged(enable);
}

void AbstractCallgrindSettings::setMinimumInclusiveCostRatio(
    double minimumInclusiveCostRatio)
{
    if (m_minimumInclusiveCostRatio == minimumInclusiveCostRatio)
        return;

    m_minimumInclusiveCostRatio = qBound(0.0, minimumInclusiveCostRatio, 100.0);
    emit minimumInclusiveCostRatioChanged(minimumInclusiveCostRatio);
}

void AbstractCallgrindSettings::setVisualisationMinimumInclusiveCostRatio(
    double minimumInclusiveCostRatio)
{
    if (m_visualisationMinimumInclusiveCostRatio == minimumInclusiveCostRatio)
        return;

    m_visualisationMinimumInclusiveCostRatio = qBound(0.0, minimumInclusiveCostRatio, 100.0);
    emit visualisationMinimumInclusiveCostRatioChanged(minimumInclusiveCostRatio);
}

QVariantMap AbstractCallgrindSettings::defaults() const
{
    QVariantMap map;
    map.insert(QLatin1String(callgrindEnableCacheSimC), false);
    map.insert(QLatin1String(callgrindEnableBranchSimC), false);
    map.insert(QLatin1String(callgrindCollectSystimeC), false);
    map.insert(QLatin1String(callgrindCollectBusEventsC), false);
    map.insert(QLatin1String(callgrindEnableEventToolTipsC), true);
    map.insert(QLatin1String(callgrindMinimumCostRatioC), 0.01);
    map.insert(QLatin1String(callgrindVisualisationMinimumCostRatioC), 10.0);
    return map;
}

bool AbstractCallgrindSettings::fromMap(const QVariantMap &map)
{
    setIfPresent(map, QLatin1String(callgrindEnableCacheSimC), &m_enableCacheSim);
    setIfPresent(map, QLatin1String(callgrindEnableBranchSimC), &m_enableBranchSim);
    setIfPresent(map, QLatin1String(callgrindCollectSystimeC), &m_collectSystime);
    setIfPresent(map, QLatin1String(callgrindCollectBusEventsC), &m_collectBusEvents);
    setIfPresent(map, QLatin1String(callgrindEnableEventToolTipsC), &m_enableEventToolTips);
    setIfPresent(map, QLatin1String(callgrindMinimumCostRatioC), &m_minimumInclusiveCostRatio);
    setIfPresent(map, QLatin1String(callgrindVisualisationMinimumCostRatioC),
                 &m_visualisationMinimumInclusiveCostRatio);
    return true;
}

QVariantMap AbstractCallgrindSettings::toMap() const
{
    QVariantMap map;
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

QString AbstractCallgrindSettings::id() const
{
    return QLatin1String("Analyzer.Valgrind.Settings.Callgrind");
}

QString AbstractCallgrindSettings::displayName() const
{
    return tr("Profiling");
}

QWidget *AbstractCallgrindSettings::createConfigWidget(QWidget *parent)
{
    return new CallgrindConfigWidget(this, parent);
}


QVariantMap CallgrindGlobalSettings::defaults() const
{
    QVariantMap map = AbstractCallgrindSettings::defaults();
    map.insert(QLatin1String(callgrindCostFormatC), CostDelegate::FormatRelative);
    map.insert(QLatin1String(callgrindCycleDetectionC), true);
    return map;
}

bool CallgrindGlobalSettings::fromMap(const QVariantMap &map)
{
    AbstractCallgrindSettings::fromMap(map);
    // special code as the default one does not cope with the enum properly
    if (map.contains(QLatin1String(callgrindCostFormatC))) {
        m_costFormat = static_cast<CostDelegate::CostFormat>(map.value(QLatin1String(callgrindCostFormatC)).toInt());
    }
    setIfPresent(map, QLatin1String(callgrindCycleDetectionC), &m_detectCycles);
    return true;
}

QVariantMap CallgrindGlobalSettings::toMap() const
{
    QVariantMap map = AbstractCallgrindSettings::toMap();
    map.insert(QLatin1String(callgrindCostFormatC), m_costFormat);
    map.insert(QLatin1String(callgrindCycleDetectionC), m_detectCycles);
    return map;
}

CostDelegate::CostFormat CallgrindGlobalSettings::costFormat() const
{
    return m_costFormat;
}

void CallgrindGlobalSettings::setCostFormat(CostDelegate::CostFormat format)
{
    m_costFormat = format;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

bool CallgrindGlobalSettings::detectCycles() const
{
    return m_detectCycles;
}

void CallgrindGlobalSettings::setDetectCycles(bool detect)
{
    m_detectCycles = detect;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

} // namespace Internal
} // namespace Callgrind
