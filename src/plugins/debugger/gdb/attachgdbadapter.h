/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef DEBUGGER_ATTACHGDBADAPTER_H
#define DEBUGGER_ATTACHGDBADAPTER_H

#include "gdbengine.h"
#include "localgdbprocess.h"

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// AttachGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbAttachEngine : public GdbEngine
{
    Q_OBJECT

public:
    explicit GdbAttachEngine(const DebuggerStartParameters &startParameters);

private:
    DumperHandling dumperHandling() const { return DumperLoadedByGdb; }

    void setupEngine();
    void setupInferior();
    void runEngine();
    void interruptInferior2();
    void shutdownEngine();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }
    void handleAttach(const GdbResponse &response);

    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_ATTACHDBADAPTER_H
