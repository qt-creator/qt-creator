/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef ANALYZERRUNCONTROL_H
#define ANALYZERRUNCONTROL_H

#include "analyzerstartparameters.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/runnables.h>

#include <utils/outputformat.h>

namespace Analyzer {

/**
 * An AnalyzerRunControl instance handles the launch of an analyzation tool.
 *
 * It gets created for each launch and deleted when the launch is stopped or ended.
 */
class ANALYZER_EXPORT AnalyzerRunControl : public ProjectExplorer::RunControl
{
    Q_OBJECT

public:
    AnalyzerRunControl(ProjectExplorer::RunConfiguration *runConfiguration,
        Core::Id runMode);

    /// Start analyzation process.
    virtual bool startEngine() = 0;
    /// Trigger async stop of the analyzation process.
    virtual void stopEngine() = 0;

    /// Controller actions.
    virtual bool canPause() const { return false; }
    virtual void pause() {}
    virtual void unpause() {}

    const ProjectExplorer::StandardRunnable &runnable() const;
    const AnalyzerConnection &connection() const;

    virtual void notifyRemoteSetupDone(quint16) {}
    virtual void notifyRemoteFinished() {}

    // ProjectExplorer::RunControl
    void start();
    StopResult stop();
    bool isRunning() const;

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &workingDirectory);

public slots:
    virtual void logApplicationMessage(const QString &, Utils::OutputFormat) {}

private slots:
    void stopIt();
    void runControlFinished();

signals:
    /// Must be emitted when the engine is starting.
    void starting();

private:
    bool supportsReRunning() const { return false; }

protected:
    bool m_isRunning;

private:
    QString m_workingDirectory;
};
} // namespace Analyzer

#endif // ANALYZERRUNCONTROL_H
