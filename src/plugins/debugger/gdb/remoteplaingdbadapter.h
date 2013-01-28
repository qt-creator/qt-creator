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

#ifndef REMOTEGDBCLIENTADAPTER_H
#define REMOTEGDBCLIENTADAPTER_H

#include "abstractplaingdbadapter.h"
#include "remotegdbprocess.h"

namespace Debugger {
namespace Internal {

class GdbRemotePlainEngine : public GdbAbstractPlainEngine
{
    Q_OBJECT

public:
    friend class RemoteGdbProcess;
    explicit GdbRemotePlainEngine(const DebuggerStartParameters &startParameters);

private slots:
    void handleGdbStarted();
    void handleGdbStartFailed1();

private:
    void setupEngine();
    void setupInferior();
    void interruptInferior2();
    void shutdownEngine();
    void notifyEngineRemoteSetupDone(int gdbServerPort, int qmlPort);
    void notifyEngineRemoteSetupFailed(const QString &reason);
    AbstractGdbProcess *gdbProc() { return &m_gdbProc; }
    DumperHandling dumperHandling() const { return DumperLoadedByGdbPreload; }

    QByteArray execFilePath() const;
    QByteArray toLocalEncoding(const QString &s) const;
    QString fromLocalEncoding(const QByteArray &b) const;
    void handleApplicationOutput(const QByteArray &output);

    RemoteGdbProcess m_gdbProc;
};

} // namespace Internal
} // namespace Debugger

#endif // REMOTEGDBCLIENTADAPTER_H
