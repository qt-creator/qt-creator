/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ANALYZERRUNCONTROL_H
#define ANALYZERRUNCONTROL_H

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/task.h>

#include "analyzerbase_global.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/task.h>
#include <projectexplorer/runconfiguration.h>
#include <utils/outputformat.h>

#include <QObject>
#include <QString>

namespace ProjectExplorer { class RunConfiguration; }

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
    AnalyzerRunControl(const AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);

    /// Start analyzation process.
    virtual bool startEngine() = 0;
    /// Trigger async stop of the analyzation process.
    virtual void stopEngine() = 0;

    /// Controller actions.
    virtual bool canPause() const { return false; }
    virtual void pause() {}
    virtual void unpause() {}

    /// The active run configuration for this engine, might be zero.
    ProjectExplorer::RunConfiguration *runConfiguration() const { return m_runConfig; }

    /// The start parameters for this engine.
    const AnalyzerStartParameters &startParameters() const { return m_sp; }

    StartMode mode() const { return m_sp.startMode; }

    virtual void notifyRemoteSetupDone(quint16) {}
    virtual void notifyRemoteFinished() {}

    bool m_isRunning;

    // ProjectExplorer::RunControl
    void start();
    StopResult stop();
    bool isRunning() const;
    QString displayName() const;
    QIcon icon() const;

public slots:
    virtual void logApplicationMessage(const QString &, Utils::OutputFormat) {}

private slots:
    void stopIt();
    void runControlFinished();

signals:
    /// Must be emitted when the engine is starting.
    void starting(const Analyzer::AnalyzerRunControl *);

private:
    ProjectExplorer::RunConfiguration *m_runConfig;
    AnalyzerStartParameters m_sp;
};
} // namespace Analyzer

#endif // ANALYZERRUNCONTROL_H
