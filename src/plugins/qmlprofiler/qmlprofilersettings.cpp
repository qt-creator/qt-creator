/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilersettings.h"
#include "qmlprofilerconstants.h"
#include "qmlprofilerconfigwidget.h"

#include <coreplugin/icore.h>

#include <QSettings>

namespace QmlProfiler {
namespace Internal {

QmlProfilerSettings::QmlProfilerSettings()
{
    QVariantMap defaults;
    defaults.insert(QLatin1String(Constants::FLUSH_INTERVAL), 1000);
    defaults.insert(QLatin1String(Constants::FLUSH_ENABLED), false);
    defaults.insert(QLatin1String(Constants::LAST_TRACE_FILE), QString());
    defaults.insert(QLatin1String(Constants::AGGREGATE_TRACES), false);

    // Read stored values
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::ANALYZER));
    QVariantMap map = defaults;
    for (QVariantMap::ConstIterator it = defaults.constBegin(); it != defaults.constEnd(); ++it)
        map.insert(it.key(), settings->value(it.key(), it.value()));
    settings->endGroup();

    fromMap(map);
}

QWidget *QmlProfilerSettings::createConfigWidget(QWidget *parent)
{
    return new Internal::QmlProfilerConfigWidget(this, parent);
}

ProjectExplorer::ISettingsAspect *QmlProfilerSettings::create() const
{
    return new QmlProfilerSettings;
}

bool QmlProfilerSettings::flushEnabled() const
{
    return m_flushEnabled;
}

void QmlProfilerSettings::setFlushEnabled(bool flushEnabled)
{
    if (m_flushEnabled != flushEnabled) {
        m_flushEnabled = flushEnabled;
        emit changed();
    }
}

quint32 QmlProfilerSettings::flushInterval() const
{
    return m_flushInterval;
}

void QmlProfilerSettings::setFlushInterval(quint32 flushInterval)
{
    if (m_flushInterval != flushInterval) {
        m_flushInterval = flushInterval;
        emit changed();
    }
}

QString QmlProfilerSettings::lastTraceFile() const
{
    return m_lastTraceFile;
}

void QmlProfilerSettings::setLastTraceFile(const QString &lastTracePath)
{
    if (m_lastTraceFile != lastTracePath) {
        m_lastTraceFile = lastTracePath;
        emit changed();
    }
}

bool QmlProfilerSettings::aggregateTraces() const
{
    return m_aggregateTraces;
}

void QmlProfilerSettings::setAggregateTraces(bool aggregateTraces)
{
    if (m_aggregateTraces != aggregateTraces) {
        m_aggregateTraces = aggregateTraces;
        emit changed();
    }
}

void QmlProfilerSettings::writeGlobalSettings() const
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::ANALYZER));
    QVariantMap map;
    toMap(map);
    for (QVariantMap::ConstIterator it = map.constBegin(); it != map.constEnd(); ++it)
        settings->setValue(it.key(), it.value());
    settings->endGroup();
}

void QmlProfilerSettings::toMap(QVariantMap &map) const
{
    map[QLatin1String(Constants::FLUSH_INTERVAL)] = m_flushInterval;
    map[QLatin1String(Constants::FLUSH_ENABLED)] = m_flushEnabled;
    map[QLatin1String(Constants::LAST_TRACE_FILE)] = m_lastTraceFile;
    map[QLatin1String(Constants::AGGREGATE_TRACES)] = m_aggregateTraces;
}

void QmlProfilerSettings::fromMap(const QVariantMap &map)
{
    m_flushEnabled = map.value(QLatin1String(Constants::FLUSH_ENABLED)).toBool();
    m_flushInterval = map.value(QLatin1String(Constants::FLUSH_INTERVAL)).toUInt();
    m_lastTraceFile = map.value(QLatin1String(Constants::LAST_TRACE_FILE)).toString();
    m_aggregateTraces = map.value(QLatin1String(Constants::AGGREGATE_TRACES)).toBool();
    emit changed();
}

} // Internal
} // QmlProfiler
