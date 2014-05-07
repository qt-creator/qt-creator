/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#ifndef VALGRINDENGINE_H
#define VALGRINDENGINE_H

#include <analyzerbase/analyzerruncontrol.h>
#include <utils/environment.h>
#include <valgrind/valgrindrunner.h>
#include <valgrind/valgrindsettings.h>

#include <QFutureInterface>
#include <QFutureWatcher>

namespace Valgrind {
namespace Internal {

class ValgrindRunControl : public Analyzer::AnalyzerRunControl
{
    Q_OBJECT

public:
    ValgrindRunControl(const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);
    ~ValgrindRunControl();

    bool startEngine();
    void stopEngine();

    QString executable() const;

protected:
    virtual QString progressTitle() const = 0;
    virtual QStringList toolArguments() const = 0;
    virtual Valgrind::ValgrindRunner *runner() = 0;

    ValgrindBaseSettings *m_settings;
    QFutureInterface<void> *m_progress;

private slots:
    void handleProgressCanceled();
    void handleProgressFinished();
    void runnerFinished();

    void receiveProcessOutput(const QString &output, Utils::OutputFormat format);
    void receiveProcessError(const QString &message, QProcess::ProcessError error);

private:
    QStringList genericToolArguments() const;

private:
    bool m_isStopping;
};

} // namespace Internal
} // namespace Valgrind

#endif // VALGRINDENGINE_H
