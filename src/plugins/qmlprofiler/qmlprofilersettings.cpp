/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
}

void QmlProfilerSettings::fromMap(const QVariantMap &map)
{
    m_flushEnabled = map.value(QLatin1String(Constants::FLUSH_ENABLED)).toBool();
    m_flushInterval = map.value(QLatin1String(Constants::FLUSH_INTERVAL)).toUInt();
    emit changed();
}

} // Internal
} // QmlProfiler
