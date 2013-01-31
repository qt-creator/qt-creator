/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
    connect(m_controller,
            SIGNAL(finished(Valgrind::Callgrind::CallgrindController::Option)),
            SLOT(controllerFinished(Valgrind::Callgrind::CallgrindController::Option)));
    connect(m_controller, SIGNAL(localParseDataAvailable(QString)),
            this, SLOT(localParseDataAvailable(QString)));
    connect(m_controller, SIGNAL(statusMessage(QString)),
            this, SIGNAL(statusMessage(QString)));
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

    ValgrindRunner::processFinished(ret, status); // call base class method
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
