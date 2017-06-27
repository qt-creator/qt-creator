/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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
#include <utils/environment.h>
#include <valgrind/valgrindrunner.h>
#include <valgrind/valgrindsettings.h>

#include <QFutureInterface>
#include <QFutureWatcher>

namespace Valgrind {
namespace Internal {

class ValgrindToolRunner : public ProjectExplorer::RunWorker
{
    Q_OBJECT

public:
    explicit ValgrindToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;
    void stop() override;

    QString executable() const;

protected:
    virtual QString progressTitle() const = 0;
    virtual QStringList toolArguments() const = 0;

    ValgrindBaseSettings *m_settings = 0;
    QFutureInterface<void> m_progress;
    ValgrindRunner m_runner;

private:
    void handleProgressCanceled();
    void handleProgressFinished();
    void runnerFinished();

    void receiveProcessOutput(const QString &output, Utils::OutputFormat format);
    void receiveProcessError(const QString &message, QProcess::ProcessError error);

    QStringList genericToolArguments() const;

private:
    bool m_isStopping = false;
};

} // namespace Internal
} // namespace Valgrind
