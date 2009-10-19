/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "attachgdbadapter.h"

#include "debuggeractions.h"
#include "gdbengine.h"
#include "procinterrupt.h"
#include "debuggerstringutils.h"

#include <utils/qtcassert.h>


namespace Debugger {
namespace Internal {

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&AttachGdbAdapter::callback), \
    STRINGIFY(callback)

///////////////////////////////////////////////////////////////////////
//
// AttachGdbAdapter
//
///////////////////////////////////////////////////////////////////////

AttachGdbAdapter::AttachGdbAdapter(GdbEngine *engine, QObject *parent)
    : AbstractGdbAdapter(engine, parent)
{
}

void AttachGdbAdapter::startAdapter()
{
    QTC_ASSERT(state() == EngineStarting, qDebug() << state());
    setState(AdapterStarting);
    debugMessage(_("TRYING TO START ADAPTER"));

    if (!m_engine->startGdb())
        return;

    emit adapterStarted();
}

void AttachGdbAdapter::startInferior()
{
    QTC_ASSERT(state() == InferiorStarting, qDebug() << state());
    const qint64 pid = startParameters().attachPID;
    m_engine->postCommand(_("attach %1").arg(pid), CB(handleAttach));
    // Task 254674 does not want to remove them
    //qq->breakHandler()->removeAllBreakpoints();
}

void AttachGdbAdapter::handleAttach(const GdbResponse &response)
{
    if (response.resultClass == GdbResultDone) {
        QTC_ASSERT(state() == InferiorStopped, qDebug() << state());
        debugMessage(_("INFERIOR ATTACHED"));
        showStatusMessage(msgAttachedToStoppedInferior());
        emit inferiorPrepared();
        m_engine->updateAll();
    } else {
        QString msg = __(response.data.findChild("msg").data());
        emit inferiorStartFailed(msg);
    }
}

void AttachGdbAdapter::interruptInferior()
{
    debugMessage(_("TRYING TO INTERUPT INFERIOR"));
    const qint64 pid = startParameters().attachPID;
    if (!interruptProcess(pid))
        debugMessage(_("CANNOT INTERRUPT %1").arg(pid));
}

} // namespace Internal
} // namespace Debugger
