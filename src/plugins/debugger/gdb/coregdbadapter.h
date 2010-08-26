/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_COREGDBADAPTER_H
#define DEBUGGER_COREGDBADAPTER_H

#include "abstractgdbadapter.h"

#include "abstractgdbprocess.h"


namespace Debugger {
namespace Internal {

///////////////////////////////////////////////////////////////////////
//
// CoreGdbAdapter
//
///////////////////////////////////////////////////////////////////////

class CoreGdbAdapter : public AbstractGdbAdapter
{
    Q_OBJECT

public:
    explicit CoreGdbAdapter(GdbEngine *engine, QObject *parent = 0);

private:
    DumperHandling dumperHandling() const { return DumperNotAvailable; }

    void startAdapter();
    void setupInferior();
    void runEngine();
    void interruptInferior();
    void shutdownInferior();
    void shutdownAdapter();

    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }

    void handleTemporaryDetach(const GdbResponse &response);
    void handleTemporaryTargetCore(const GdbResponse &response);
    void handleFileExecAndSymbols(const GdbResponse &response);
    void handleTargetCore(const GdbResponse &response);

    QString m_executable;
    const QByteArray m_coreName;
    LocalGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_COREDBADAPTER_H
