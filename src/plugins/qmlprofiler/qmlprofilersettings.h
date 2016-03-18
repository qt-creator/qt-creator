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

#pragma once

#include <projectexplorer/runconfiguration.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerSettings : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT
public:
    QmlProfilerSettings();
    QWidget *createConfigWidget(QWidget *parent);
    ProjectExplorer::ISettingsAspect *create() const;

    bool flushEnabled() const;
    void setFlushEnabled(bool flushEnabled);

    quint32 flushInterval() const;
    void setFlushInterval(quint32 flushInterval);

    QString lastTraceFile() const;
    void setLastTraceFile(const QString &lastTraceFile);

    bool aggregateTraces() const;
    void setAggregateTraces(bool aggregateTraces);

    void writeGlobalSettings() const;

signals:
    void changed();

protected:
    void toMap(QVariantMap &map) const;
    void fromMap(const QVariantMap &map);

private:
    bool m_flushEnabled;
    quint32 m_flushInterval;
    QString m_lastTraceFile;
    bool m_aggregateTraces;
};

} // Internal
} // QmlProfiler
