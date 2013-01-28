/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#ifndef ANDROIDDEBUGSUPPORT_H
#define ANDROIDDEBUGSUPPORT_H

#include "androidrunconfiguration.h"

namespace Debugger { class DebuggerRunControl; }
namespace ProjectExplorer { class RunControl; }

namespace Android {
namespace Internal {

class AndroidRunConfiguration;
class AndroidRunner;

class AndroidDebugSupport : public QObject
{
    Q_OBJECT

public:
    static ProjectExplorer::RunControl *createDebugRunControl(AndroidRunConfiguration *runConfig,
                                                              QString *errorMessage);

    AndroidDebugSupport(AndroidRunConfiguration *runConfig,
        Debugger::DebuggerRunControl *runControl);

private slots:
    void handleRemoteProcessStarted(int gdbServerPort = -1, int qmlPort = -1);
    void handleRemoteProcessFinished(const QString &errorMsg);

    void handleRemoteOutput(const QByteArray &output);
    void handleRemoteErrorOutput(const QByteArray &output);

private:
    Debugger::DebuggerRunControl* m_runControl;
    AndroidRunner * const m_runner;
    const QString m_dumperLib;

    int m_gdbServerPort;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Android

#endif // ANDROIDDEBUGSUPPORT_H
