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

#ifndef DEBUGGER_PLAINGDBADAPTER_H
#define DEBUGGER_PLAINGDBADAPTER_H

#include "abstractplaingdbadapter.h"
#include "localgdbprocess.h"
#include <outputcollector.h>

namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// PlainGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class GdbLocalPlainEngine : public GdbAbstractPlainEngine
{
    Q_OBJECT

public:
    GdbLocalPlainEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine);

private:
    void setupEngine();
    void handleGdbStartFailed();
    void interruptInferior2();
    void shutdownEngine();

    DumperHandling dumperHandling() const;
    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    QByteArray execFilePath() const;
    QByteArray toLocalEncoding(const QString &s) const;
    QString fromLocalEncoding(const QByteArray &b) const;
    void checkForReleaseBuild();

    OutputCollector m_outputCollector;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_PLAINGDBADAPTER_H
