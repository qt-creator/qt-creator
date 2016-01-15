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

#include "callgrindrunner.h"
#include "callgrindparser.h"

#include <utils/qtcassert.h>

#include <QFile>

namespace Valgrind {
namespace Callgrind {

CallgrindRunner::CallgrindRunner(QObject *parent)
    : ValgrindRunner(parent)
    , m_controller(new CallgrindController(this))
    , m_parser(new Parser(this))
    , m_paused(false)
{
    connect(m_controller, &CallgrindController::finished,
            this, &CallgrindRunner::controllerFinished);
    connect(m_controller, &CallgrindController::localParseDataAvailable,
            this, &CallgrindRunner::localParseDataAvailable);
    connect(m_controller, &CallgrindController::statusMessage,
            this, &CallgrindRunner::statusMessage);
}

QString CallgrindRunner::tool() const
{
    return QLatin1String("callgrind");
}

Parser *CallgrindRunner::parser() const
{
    return m_parser;
}

CallgrindController *CallgrindRunner::controller() const
{
    return m_controller;
}

bool CallgrindRunner::start()
{
    ValgrindRunner::start();
    m_controller->setValgrindProcess(valgrindProcess());
    return true;
}

void CallgrindRunner::processFinished(int ret, QProcess::ExitStatus status)
{
    triggerParse();
    m_controller->setValgrindProcess(0);

    ValgrindRunner::processFinished(ret, status); // call base class function
}

bool CallgrindRunner::isPaused() const
{
    return m_paused;
}

void CallgrindRunner::triggerParse()
{
    m_controller->getLocalDataFile();
}

void CallgrindRunner::localParseDataAvailable(const QString &file)
{
    // parse the callgrind file
    QTC_ASSERT(!file.isEmpty(), return);
    QFile outputFile(file);
    QTC_ASSERT(outputFile.exists(), return);
    if (outputFile.open(QIODevice::ReadOnly)) {
        emit statusMessage(tr("Parsing Profile Data..."));
        m_parser->parse(&outputFile);
    } else {
        qWarning() << "Could not open file for parsing:" << outputFile.fileName();
    }
}

void CallgrindRunner::controllerFinished(CallgrindController::Option option)
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

} // namespace Callgrind
} // namespace Valgrind
