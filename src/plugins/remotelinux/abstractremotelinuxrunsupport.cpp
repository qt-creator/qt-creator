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

#include "abstractremotelinuxrunsupport.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runnables.h>
#include <projectexplorer/target.h>

#include <utils/environment.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

// FifoGatherer

FifoGatherer::FifoGatherer(RunControl *runControl)
    : RunWorker(runControl)
{
    setDisplayName("FifoGatherer");
}

FifoGatherer::~FifoGatherer()
{
}

void FifoGatherer::start()
{
    appendMessage(tr("Creating remote socket...") + '\n', NormalMessageFormat);

    StandardRunnable r;
    r.executable = QLatin1String("/bin/sh");
    r.commandLineArguments = "-c 'd=`mktemp -d` && mkfifo $d/fifo && echo -n $d/fifo'";
    r.workingDirectory = QLatin1String("/tmp");
    r.runMode = ApplicationLauncher::Console;

    QSharedPointer<QString> output(new QString);
    QSharedPointer<QString> errors(new QString);

    connect(&m_fifoCreator, &ApplicationLauncher::finished,
            this, [this, output, errors](bool success) {
        if (!success) {
            reportFailure(QString("Failed to create fifo: %1").arg(*errors));
        } else {
            m_fifo = *output;
            appendMessage(tr("Created fifo: %1").arg(m_fifo), NormalMessageFormat);
            reportStarted();
        }
    });

    connect(&m_fifoCreator, &ApplicationLauncher::remoteStdout,
            this, [output](const QString &data) {
        output->append(data);
    });

    connect(&m_fifoCreator, &ApplicationLauncher::remoteStderr,
            this, [this, errors](const QString &) {
            reportFailure();
//        errors->append(data);
    });

    m_fifoCreator.start(r, device());
}

void FifoGatherer::stop()
{
    m_fifoCreator.stop();
    reportStopped();
}

} // namespace RemoteLinux
