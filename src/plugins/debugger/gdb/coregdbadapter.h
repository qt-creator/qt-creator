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

#ifndef DEBUGGER_COREGDBADAPTER_H
#define DEBUGGER_COREGDBADAPTER_H

#include "abstractgdbadapter.h"
#include "gdbengine.h"

#include <QtCore/QDebug>
#include <QtCore/QProcess>

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
    CoreGdbAdapter(GdbEngine *engine, QObject *parent = 0);

private:
    QByteArray readAllStandardError() { return m_gdbProc.readAllStandardError(); }
    QByteArray readAllStandardOutput() { return m_gdbProc.readAllStandardOutput(); }
    void write(const QByteArray &data) { m_gdbProc.write(data, data.size()); }
    void setWorkingDirectory(const QString &dir) { m_gdbProc.setWorkingDirectory(dir); }
    void setEnvironment(const QStringList &env) { m_gdbProc.setEnvironment(env); }
    bool isTrkAdapter() const { return false; }
    bool dumpersAvailable() const { return false; }

    void startAdapter();
    void prepareInferior();
    void startInferior();
    void interruptInferior();
    void shutdown();

    void handleFileExecAndSymbols(const GdbResponse &response);
    void handleTargetCore(const GdbResponse &response);
    void handleExit(const GdbResponse &response);

    Q_SLOT void handleGdbStarted();
    Q_SLOT void handleGdbError(QProcess::ProcessError error);
    Q_SLOT void handleGdbFinished(int, QProcess::ExitStatus);

    QProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // DEBUGGER_COREDBADAPTER_H
