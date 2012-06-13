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
    GdbRemotePlainEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine);

private slots:
    void handleGdbStarted();
    void handleGdbStartFailed1();

private:
    void setupEngine();
    void setupInferior();
    void interruptInferior2();
    void shutdownEngine();
    void handleRemoteSetupDone(int gdbServerPort, int qmlPort);
    void handleRemoteSetupFailed(const QString &reason);
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
