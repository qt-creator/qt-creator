/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Andreas Hartmetz, KDAB (andreas.hartmetz@kdab.com)
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

#ifndef ANALYZERRUNCONTROL_H
#define ANALYZERRUNCONTROL_H

#include "analyzerbase_global.h"

#include <projectexplorer/runconfiguration.h>

#include "analyzerstartparameters.h"

#include <utils/outputformat.h>

#include <QObject>
#include <QString>

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

    /// The start parameters for this engine.
    const AnalyzerStartParameters &startParameters() const { return m_sp; }

    virtual void notifyRemoteSetupDone(quint16) {}
    virtual void notifyRemoteFinished() {}

    bool m_isRunning;

    // ProjectExplorer::RunControl
    void start();
    StopResult stop();
    bool isRunning() const;
    QString displayName() const;

public slots:
    virtual void logApplicationMessage(const QString &, Utils::OutputFormat) {}

private slots:
    void stopIt();
    void runControlFinished();

signals:
    /// Must be emitted when the engine is starting.
    void starting(const Analyzer::AnalyzerRunControl *);

private:
    bool supportsReRunning() const { return false; }
    AnalyzerStartParameters m_sp;
};
} // namespace Analyzer

#endif // ANALYZERRUNCONTROL_H
