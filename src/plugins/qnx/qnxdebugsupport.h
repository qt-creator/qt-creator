/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 - 2012 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QNX_INTERNAL_QNXDEBUGSUPPORT_H
#define QNX_INTERNAL_QNXDEBUGSUPPORT_H

#include <QObject>

namespace Debugger {
class DebuggerEngine;
}

namespace Qnx {
namespace Internal {

class QnxApplicationRunner;
class QnxRunConfiguration;

class QnxDebugSupport : public QObject
{
    Q_OBJECT
public:
    explicit QnxDebugSupport(QnxRunConfiguration *runConfig,
                             Debugger::DebuggerEngine *engine);

public slots:
    void handleDebuggingFinished();

private slots:
    void handleAdapterSetupRequested();

    void startExecution();
    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(qint64 exitCode);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteOutput(const QByteArray &output);
    void handleSshError(const QString &error);

private:
    void setFinished();

    enum State {
        Inactive,
        StartingRunner,
        StartingRemoteProcess,
        Debugging
    };

    QnxApplicationRunner *m_runner;

    Debugger::DebuggerEngine *m_engine;
    int m_port;

    State m_state;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXDEBUGSUPPORT_H
