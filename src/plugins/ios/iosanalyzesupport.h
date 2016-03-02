/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef IOSANALYZESUPPORT_H
#define IOSANALYZESUPPORT_H

#include "iosrunconfiguration.h"

#include <qmldebug/qmloutputparser.h>

#include <QProcess>
#include <QObject>

namespace Debugger { class AnalyzerRunControl; }

namespace Ios {
namespace Internal {

class IosRunConfiguration;
class IosRunner;

class IosAnalyzeSupport : public QObject
{
    Q_OBJECT

public:
    IosAnalyzeSupport(IosRunConfiguration *runConfig,
                      Debugger::AnalyzerRunControl *runControl, bool cppDebug, bool qmlDebug);
    ~IosAnalyzeSupport();

private:
    void qmlServerReady();
    void handleServerPorts(int gdbServerFd, int qmlPort);
    void handleGotInferiorPid(qint64 pid, int qmlPort);
    void handleRemoteProcessFinished(bool cleanEnd);

    void handleRemoteOutput(const QString &output);
    void handleRemoteErrorOutput(const QString &output);

    Debugger::AnalyzerRunControl *m_runControl;
    IosRunner * const m_runner;
    QmlDebug::QmlOutputParser m_outputParser;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Ios

#endif // IOSANALYZESUPPORT_H
