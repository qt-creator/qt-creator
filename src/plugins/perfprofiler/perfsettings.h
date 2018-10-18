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

#pragma once

#include "perfprofiler_global.h"

#include <projectexplorer/runconfiguration.h>

#include <QObject>

namespace PerfProfiler {

class PERFPROFILER_EXPORT PerfSettings : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT
    Q_PROPERTY(QStringList perfRecordArguments READ perfRecordArguments NOTIFY changed)

public:
    explicit PerfSettings(ProjectExplorer::Target *target = nullptr);
    ~PerfSettings();

    void readGlobalSettings();
    void writeGlobalSettings() const;

    int period() const;
    int stackSize() const;
    QString sampleMode() const;
    QString callgraphMode() const;
    QStringList events() const;
    QStringList extraArguments() const;

    QStringList perfRecordArguments() const;

    void setPeriod(int period);
    void setStackSize(int stackSize);
    void setSampleMode(const QString &sampleMode);
    void setCallgraphMode(const QString &callgraphMode);
    void setEvents(const QStringList &events);
    void setExtraArguments(const QStringList &args);
    void resetToDefault();

signals:
    void changed();

protected:
    void toMap(QVariantMap &map) const;
    void fromMap(const QVariantMap &map);

    int m_period;
    int m_stackSize;
    QString m_sampleMode;
    QString m_callgraphMode;
    QStringList m_events;
    QStringList m_extraArguments;
};

} // namespace PerfProfiler
