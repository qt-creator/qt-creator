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
using namespace Callgrind::Internal;
using namespace Callgrind;

static const char callgrindEnableCacheSimC[] = "Analyzer.Valgrind.Callgrind.EnableCacheSim";
static const char callgrindEnableBranchSimC[] = "Analyzer.Valgrind.Callgrind.EnableBranchSim";
static const char callgrindCollectSystimeC[] = "Analyzer.Valgrind.Callgrind.CollectSystime";
static const char callgrindCollectBusEventsC[] = "Analyzer.Valgrind.Callgrind.CollectBusEvents";

static const char callgrindCycleDetectionC[] = "Analyzer.Valgrind.Callgrind.CycleDetection";
static const char callgrindCostFormatC[] = "Analyzer.Valgrind.Callgrind.CostFormat";
static const char callgrindMinimumCostRatioC[] = "Analyzer.Valgrind.Callgrind.MinimumCostRatio";

AbstractCallgrindSettings::AbstractCallgrindSettings(QObject *parent)
    : AbstractAnalyzerSubConfig(parent)
{

}

AbstractCallgrindSettings::~AbstractCallgrindSettings()
{

}

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

QVariantMap AbstractCallgrindSettings::defaults() const
{
    QVariantMap map;
    map.insert(QLatin1String(callgrindEnableCacheSimC), false);
    map.insert(QLatin1String(callgrindEnableBranchSimC), false);
    map.insert(QLatin1String(callgrindCollectSystimeC), false);
    map.insert(QLatin1String(callgrindCollectBusEventsC), false);
    return map;
}

bool AbstractCallgrindSettings::fromMap(const QVariantMap &map)
{
    setIfPresent(map, QLatin1String(callgrindEnableCacheSimC), &m_enableCacheSim);
    setIfPresent(map, QLatin1String(callgrindEnableBranchSimC), &m_enableBranchSim);
    setIfPresent(map, QLatin1String(callgrindCollectSystimeC), &m_collectSystime);
    setIfPresent(map, QLatin1String(callgrindCollectBusEventsC), &m_collectBusEvents);
    return true;
}

QVariantMap AbstractCallgrindSettings::toMap() const
{
    QVariantMap map;
    map.insert(QLatin1String(callgrindEnableCacheSimC), m_enableCacheSim);
    map.insert(QLatin1String(callgrindEnableBranchSimC), m_enableBranchSim);
    map.insert(QLatin1String(callgrindCollectSystimeC), m_collectSystime);
    map.insert(QLatin1String(callgrindCollectBusEventsC), m_collectBusEvents);

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


CallgrindGlobalSettings::CallgrindGlobalSettings(QObject *parent)
    : AbstractCallgrindSettings(parent)
{

}

CallgrindGlobalSettings::~CallgrindGlobalSettings()
{

}

QVariantMap CallgrindGlobalSettings::defaults() const
{
    QVariantMap map = AbstractCallgrindSettings::defaults();
    map.insert(QLatin1String(callgrindCostFormatC), CostDelegate::FormatRelative);
    map.insert(QLatin1String(callgrindCycleDetectionC), true);
    map.insert(QLatin1String(callgrindMinimumCostRatioC), 0.0001);
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
    setIfPresent(map, QLatin1String(callgrindMinimumCostRatioC), &m_minimumInclusiveCostRatio);
    return true;
}

QVariantMap CallgrindGlobalSettings::toMap() const
{
    QVariantMap map = AbstractCallgrindSettings::toMap();
    map.insert(QLatin1String(callgrindCostFormatC), m_costFormat);
    map.insert(QLatin1String(callgrindCycleDetectionC), m_detectCycles);
    map.insert(QLatin1String(callgrindMinimumCostRatioC), m_minimumInclusiveCostRatio);
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

double CallgrindGlobalSettings::minimumInclusiveCostRatio() const
{
    return m_minimumInclusiveCostRatio;
}

void CallgrindGlobalSettings::setMinimumInclusiveCostRatio(double minimumInclusiveCost)
{
    m_minimumInclusiveCostRatio = minimumInclusiveCost;
    AnalyzerGlobalSettings::instance()->writeSettings();
}

CallgrindProjectSettings::CallgrindProjectSettings(QObject *parent)
    : AbstractCallgrindSettings(parent)
{

}

CallgrindProjectSettings::~CallgrindProjectSettings()
{

}
