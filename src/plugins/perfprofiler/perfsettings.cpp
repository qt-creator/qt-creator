/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "perfconfigwidget.h"
#include "perfprofilerconstants.h"
#include "perfsettings.h"

#include <coreplugin/icore.h>

#include <QSettings>

namespace PerfProfiler {

PerfSettings::PerfSettings(ProjectExplorer::Target *target)
    : ISettingsAspect([this, target] {
        auto widget = new Internal::PerfConfigWidget(this);
        widget->setTracePointsButtonVisible(target != nullptr);
        widget->setTarget(target);
        return widget;
    })
{
    readGlobalSettings();
}

PerfSettings::~PerfSettings()
{
}

void PerfSettings::readGlobalSettings()
{
    QVariantMap defaults;
    defaults.insert(QLatin1String(Constants::PerfEventsId), QStringList({"cpu-cycles"}));
    defaults.insert(QLatin1String(Constants::PerfSampleModeId), Constants::PerfSampleFrequency);
    defaults.insert(QLatin1String(Constants::PerfFrequencyId), Constants::PerfDefaultPeriod);
    defaults.insert(QLatin1String(Constants::PerfStackSizeId), Constants::PerfDefaultStackSize);
    defaults.insert(QLatin1String(Constants::PerfCallgraphModeId),
                    QLatin1String(Constants::PerfCallgraphDwarf));
    defaults.insert(QLatin1String(Constants::PerfExtraArgumentsId), QStringList());

    // Read stored values
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::AnalyzerSettingsGroupId));
    QVariantMap map = defaults;
    for (QVariantMap::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    fromMap(map);
}

void PerfSettings::writeGlobalSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::AnalyzerSettingsGroupId));
    QVariantMap map;
    toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
}

void PerfSettings::toMap(QVariantMap &map) const
{
    map[QLatin1String(Constants::PerfSampleModeId)] = m_sampleMode;
    map[QLatin1String(Constants::PerfFrequencyId)] = m_period;
    map[QLatin1String(Constants::PerfStackSizeId)] = m_stackSize;
    map[QLatin1String(Constants::PerfCallgraphModeId)] = m_callgraphMode;
    map[QLatin1String(Constants::PerfEventsId)] = m_events;
    map[QLatin1String(Constants::PerfExtraArgumentsId)] = m_extraArguments;
}

void PerfSettings::fromMap(const QVariantMap &map)
{
    m_sampleMode = map.value(QLatin1String(Constants::PerfSampleModeId), m_sampleMode).toString();
    m_period = map.value(QLatin1String(Constants::PerfFrequencyId), m_period).toInt();
    m_stackSize = map.value(QLatin1String(Constants::PerfStackSizeId), m_stackSize).toInt();
    m_callgraphMode = map.value(QLatin1String(Constants::PerfCallgraphModeId),
                                m_callgraphMode).toString();
    m_events = map.value(QLatin1String(Constants::PerfEventsId), m_events).toStringList();
    m_extraArguments = map.value(QLatin1String(Constants::PerfExtraArgumentsId),
                                 m_extraArguments).toStringList();
    emit changed();
}

QString PerfSettings::callgraphMode() const
{
    return m_callgraphMode;
}

QStringList PerfSettings::events() const
{
    return m_events;
}

QStringList PerfSettings::extraArguments() const
{
    return m_extraArguments;
}

QStringList PerfSettings::perfRecordArguments() const
{
    QString callgraphArg = m_callgraphMode;
    if (m_callgraphMode == QLatin1String(Constants::PerfCallgraphDwarf))
        callgraphArg += "," + QString::number(m_stackSize);

    QString events;
    for (const QString &event : m_events) {
        if (!event.isEmpty()) {
            if (!events.isEmpty())
                events.append(',');
            events.append(event);
        }
    }

    return QStringList({"-e", events, "--call-graph", callgraphArg, m_sampleMode,
                        QString::number(m_period)}) + m_extraArguments;
}

void PerfSettings::setCallgraphMode(const QString &callgraphMode)
{
    m_callgraphMode = callgraphMode;
}

void PerfSettings::setEvents(const QStringList &events)
{
    m_events = events;
}

void PerfSettings::setExtraArguments(const QStringList &args)
{
    m_extraArguments = args;
}

void PerfSettings::resetToDefault()
{
    PerfSettings defaults;
    QVariantMap map;
    defaults.toMap(map);
    fromMap(map);
}

int PerfSettings::stackSize() const
{
    return m_stackSize;
}

QString PerfSettings::sampleMode() const
{
    return m_sampleMode;
}

void PerfSettings::setStackSize(int stackSize)
{
    m_stackSize = stackSize;
}

void PerfSettings::setSampleMode(const QString &sampleMode)
{
    m_sampleMode = sampleMode;
}

int PerfSettings::period() const
{
    return m_period;
}

void PerfSettings::setPeriod(int period)
{
    m_period = period;
}

} // namespace PerfProfiler
