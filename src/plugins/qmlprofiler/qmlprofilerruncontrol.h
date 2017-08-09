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

#include "qmlprofilerstatemanager.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>

#include <utils/outputformat.h>
#include <utils/port.h>

#include <qmldebug/qmloutputparser.h>

namespace QmlProfiler {

class QmlProfilerRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    QmlProfilerRunner(ProjectExplorer::RunControl *runControl);
    ~QmlProfilerRunner() override;

    void setServerUrl(const QUrl &serverUrl);
    QUrl serverUrl() const;

    void registerProfilerStateManager( QmlProfilerStateManager *profilerState );

    void cancelProcess();
    void notifyRemoteFinished();

private:
    void start() override;
    void stop() override;

    void profilerStateChanged();

    class QmlProfilerRunnerPrivate;
    QmlProfilerRunnerPrivate *d;
};

class LocalQmlProfilerSupport : public ProjectExplorer::SimpleTargetRunner
{
    Q_OBJECT

public:
    LocalQmlProfilerSupport(ProjectExplorer::RunControl *runControl);
    LocalQmlProfilerSupport(ProjectExplorer::RunControl *runControl,
                            const QUrl &serverUrl);

private:
    QmlProfilerRunner *m_profiler;
};

} // namespace QmlProfiler
