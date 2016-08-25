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

#include "qmlprofiler_global.h"
#include <utils/environment.h>
#include <utils/port.h>
#include <projectexplorer/applicationlauncher.h>
#include <projectexplorer/runnables.h>
#include <qmldebug/qmloutputparser.h>

namespace Debugger {
class AnalyzerRunControl;
}

namespace QmlProfiler {

class QMLPROFILER_EXPORT LocalQmlProfilerRunner : public QObject
{
    Q_OBJECT

public:
    struct Configuration {
        ProjectExplorer::StandardRunnable debuggee;
        Utils::Port port;
        QString socket;
    };

    LocalQmlProfilerRunner(const Configuration &configuration,
                           Debugger::AnalyzerRunControl *runControl);

    static Utils::Port findFreePort(QString &host);
    static QString findFreeSocket();

signals:
    void started();
    void stopped();
    void appendMessage(const QString &message, Utils::OutputFormat format);

private:
    void spontaneousStop(int exitCode, QProcess::ExitStatus status);
    void start();
    void stop();

    Configuration m_configuration;
    ProjectExplorer::ApplicationLauncher m_launcher;
    QmlDebug::QmlOutputParser m_outputParser;
};

} // namespace QmlProfiler
