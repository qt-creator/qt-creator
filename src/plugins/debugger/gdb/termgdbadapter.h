/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef DEBUGGER_TERMGDBADAPTER_H
#define DEBUGGER_TERMGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "localgdbprocess.h"

#include <utils/consoleprocess.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// TermGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class TermGdbAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    explicit TermGdbAdapter(GdbEngine *engine, QObject *parent = 0);
    ~TermGdbAdapter();

private:
    DumperHandling dumperHandling() const;

    void startAdapter();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void handleStubAttached(const GdbResponse &response);
#ifdef Q_OS_LINUX
    void handleEntryPoint(const GdbResponse &response);
#endif

    Q_SLOT void handleInferiorSetupOk();
    Q_SLOT void stubExited();
    Q_SLOT void stubMessage(const QString &msg, bool isError);

    Utils::ConsoleProcess m_stubProc;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_TERMGDBADAPTER_H
