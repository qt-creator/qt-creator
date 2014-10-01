/**************************************************************************
**
** Copyright (C) 2012 - 2014 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QNX_INTERNAL_BLACKBERRYDEBUGSUPPORT_H
#define QNX_INTERNAL_BLACKBERRYDEBUGSUPPORT_H

#include <debugger/debuggerconstants.h>

#include <utils/outputformat.h>

#include <QObject>
#include <QProcess>

namespace Debugger {
class DebuggerEngine;
class DebuggerRunControl;
}

namespace Qnx {
namespace Internal {

class BlackBerryConnect;
class BlackBerryRunConfiguration;
class BlackBerryApplicationRunner;

class BlackBerryDebugSupport : public QObject
{
    Q_OBJECT
public:
    explicit BlackBerryDebugSupport(BlackBerryRunConfiguration *runConfig,
                             Debugger::DebuggerRunControl *runControl);

signals:
    void output(const QString &msg, Utils::OutputFormat format);

private slots:
    void launchRemoteApplication();

    void handleStarted();
    void handleStartFailed(const QString &message);

    void handleDebuggerStateChanged(Debugger::DebuggerState state);

    void handleApplicationOutput(const QString &msg, Utils::OutputFormat format);

private:
    Debugger::DebuggerEngine *m_engine;

    BlackBerryApplicationRunner *m_runner;
};

} // namespace Internal
} // namespace Qnx

#endif // QNX_INTERNAL_BLACKBERRYDEBUGSUPPORT_H
