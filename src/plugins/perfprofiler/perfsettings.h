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

class PERFPROFILER_EXPORT PerfSettings final : public ProjectExplorer::ISettingsAspect
{
    Q_OBJECT
    Q_PROPERTY(QStringList perfRecordArguments READ perfRecordArguments NOTIFY changed)

public:
    explicit PerfSettings(ProjectExplorer::Target *target = nullptr);
    ~PerfSettings() final;

    void readGlobalSettings();
    void writeGlobalSettings() const;

    QStringList perfRecordArguments() const;

    void resetToDefault();

    Utils::IntegerAspect period;
    Utils::IntegerAspect stackSize;
    Utils::SelectionAspect sampleMode;
    Utils::SelectionAspect callgraphMode;
    Utils::StringListAspect events;
    Utils::StringAspect extraArguments;

signals:
    void changed();
};

} // namespace PerfProfiler
