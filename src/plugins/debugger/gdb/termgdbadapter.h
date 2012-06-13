/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_TERMGDBADAPTER_H
#define DEBUGGER_TERMGDBADAPTER_H

#include "gdbengine.h"
#include "localgdbprocess.h"

#include <utils/consoleprocess.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// TermGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbTermEngine : public GdbEngine
{
    Q_OBJECT

public:
    GdbTermEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine);

    ~GdbTermEngine();

private:
    DumperHandling dumperHandling() const;

    void setupEngine();
    void handleGdbStartFailed();
    void setupInferior();
    void runEngine();
    void interruptInferior2();
    void shutdownEngine();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void handleStubAttached(const GdbResponse &response);

    Q_SLOT void stubStarted();
    Q_SLOT void stubExited();
    Q_SLOT void stubError(const QString &msg);

    Utils::ConsoleProcess m_stubProc;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_TERMGDBADAPTER_H
