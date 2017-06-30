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

#include "callgrindengine.h"

#include "callgrindtool.h"
#include "valgrindsettings.h"

#include <valgrind/callgrind/callgrindcontroller.h>
#include <valgrind/callgrind/callgrindparser.h>
#include <valgrind/valgrindrunner.h>

#include <debugger/analyzer/analyzermanager.h>

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Valgrind::Callgrind;

namespace Valgrind {
namespace Internal {

CallgrindToolRunner::CallgrindToolRunner(RunControl *runControl)
    : ValgrindToolRunner(runControl)
{
    setDisplayName("CallgrindToolRunner");

    connect(&m_runner, &ValgrindRunner::finished,
            this, &CallgrindToolRunner::slotFinished);
    connect(&m_parser, &Callgrind::Parser::parserDataReady,
            this, &CallgrindToolRunner::slotFinished);

    connect(&m_controller, &CallgrindController::finished,
            this, &CallgrindToolRunner::controllerFinished);
    connect(&m_controller, &CallgrindController::localParseDataAvailable,
            this, &CallgrindToolRunner::localParseDataAvailable);
    connect(&m_controller, &CallgrindController::statusMessage,
            this, &CallgrindToolRunner::showStatusMessage);

    connect(&m_runner, &ValgrindRunner::valgrindStarted,
            &m_controller, &CallgrindController::setValgrindPid);

    connect(&m_runner, &ValgrindRunner::extraProcessFinished, this, [this] {
        triggerParse();
    });

    m_controller.setValgrindRunnable(runnable());
}

QStringList CallgrindToolRunner::toolArguments() const
{
    QStringList arguments = {"--tool=callgrind"};

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->enableCacheSim())
        arguments << QLatin1String("--cache-sim=yes");

    if (m_settings->enableBranchSim())
        arguments << QLatin1String("--branch-sim=yes");

    if (m_settings->collectBusEvents())
        arguments << QLatin1String("--collect-bus=yes");

    if (m_settings->collectSystime())
        arguments << QLatin1String("--collect-systime=yes");

    if (m_markAsPaused)
        arguments << QLatin1String("--instr-atstart=no");

    // add extra arguments
    if (!m_argumentForToggleCollect.isEmpty())
        arguments << m_argumentForToggleCollect;

    return arguments;
}

QString CallgrindToolRunner::progressTitle() const
{
    return tr("Profiling");
}

void CallgrindToolRunner::start()
{
    appendMessage(tr("Profiling %1").arg(executable()), Utils::NormalMessageFormat);
    return ValgrindToolRunner::start();
}

void CallgrindToolRunner::dump()
{
    m_controller.run(CallgrindController::Dump);
}

void CallgrindToolRunner::setPaused(bool paused)
{
    if (m_markAsPaused == paused)
        return;

    m_markAsPaused = paused;

    // call controller only if it is attached to a valgrind process
    if (paused)
        pause();
    else
        unpause();
}

void CallgrindToolRunner::setToggleCollectFunction(const QString &toggleCollectFunction)
{
    if (toggleCollectFunction.isEmpty())
        return;

    m_argumentForToggleCollect = QLatin1String("--toggle-collect=") + toggleCollectFunction;
}

void CallgrindToolRunner::reset()
{
    m_controller.run(Callgrind::CallgrindController::ResetEventCounters);
}

void CallgrindToolRunner::pause()
{
    m_controller.run(Callgrind::CallgrindController::Pause);
}

void CallgrindToolRunner::unpause()
{
    m_controller.run(Callgrind::CallgrindController::UnPause);
}

Callgrind::ParseData *CallgrindToolRunner::takeParserData()
{
    return m_parser.takeData();
}

void CallgrindToolRunner::slotFinished()
{
    emit parserDataReady(this);
}

void CallgrindToolRunner::showStatusMessage(const QString &message)
{
    Debugger::showPermanentStatusMessage(message);
}

void CallgrindToolRunner::triggerParse()
{
    m_controller.getLocalDataFile();
}

void CallgrindToolRunner::localParseDataAvailable(const QString &file)
{
    // parse the callgrind file
    QTC_ASSERT(!file.isEmpty(), return);
    QFile outputFile(file);
    QTC_ASSERT(outputFile.exists(), return);
    if (outputFile.open(QIODevice::ReadOnly)) {
        showStatusMessage(tr("Parsing Profile Data..."));
        m_parser.parse(&outputFile);
    } else {
        qWarning() << "Could not open file for parsing:" << outputFile.fileName();
    }
}

void CallgrindToolRunner::controllerFinished(CallgrindController::Option option)
{
    switch (option)
    {
    case CallgrindController::Pause:
        m_paused = true;
        break;
    case CallgrindController::UnPause:
        m_paused = false;
        break;
    case CallgrindController::Dump:
        triggerParse();
        break;
    default:
        break; // do nothing
    }
}

} // Internal
} // Valgrind
