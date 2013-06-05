/**************************************************************************
**
** Copyright (C) 2011 - 2013 Research In Motion
**
** Contact: Research In Motion (blackberry-qt@qnx.com)
** Contact: KDAB (info@kdab.com)
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

#ifndef QNX_INTERNAL_QNXDEBUGSUPPORT_H
#define QNX_INTERNAL_QNXDEBUGSUPPORT_H

#include "qnxabstractrunsupport.h"

namespace Debugger { class DebuggerEngine; }

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;

class QnxDebugSupport : public QnxAbstractRunSupport
{
    Q_OBJECT
public:
    explicit QnxDebugSupport(QnxRunConfiguration *runConfig,
                             Debugger::DebuggerEngine *engine);

public slots:
    void handleDebuggingFinished();

private slots:
    void handleAdapterSetupRequested();

    void handleRemoteProcessStarted();
    void handleRemoteProcessFinished(bool success);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteOutput(const QByteArray &output);
    void handleError(const QString &error);

private:
    void startExecution();

    QString executable() const;

    Debugger::DebuggerEngine *m_engine;
    int m_pdebugPort;
    int m_qmlPort;

    bool m_useCppDebugger;
    bool m_useQmlDebugger;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_QNXDEBUGSUPPORT_H
