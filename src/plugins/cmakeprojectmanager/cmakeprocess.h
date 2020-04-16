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

#include "builddirparameters.h"

#include <utils/outputformatter.h>
#include <utils/qtcprocess.h>

#include <QElapsedTimer>
#include <QFutureInterface>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <memory>

namespace CMakeProjectManager {
namespace Internal {

class CMakeProcess : public QObject {
    Q_OBJECT

public:
    CMakeProcess();
    CMakeProcess(const CMakeProcess&) = delete;
    ~CMakeProcess();

    void run(const BuildDirParameters &parameters, const QStringList &arguments);

    QProcess::ProcessState state() const;

    // Update progress information:
    void reportCanceled();
    void reportFinished(); // None of the progress related functions will work after this!
    void setProgressValue(int p);

    // Process stdout/stderr:
    void processStandardOutput();
    void processStandardError();

signals:
    void started();
    void finished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void handleProcessFinished(int code, QProcess::ExitStatus status);
    void checkForCancelled();

    std::unique_ptr<Utils::QtcProcess> m_process;
    Utils::OutputFormatter m_parser;
    std::unique_ptr<QFutureInterface<void>> m_future;
    bool m_processWasCanceled = false;
    QTimer m_cancelTimer;
    QElapsedTimer m_elapsed;
};

} // namespace Internal
} // namespace CMakeProjectManager
