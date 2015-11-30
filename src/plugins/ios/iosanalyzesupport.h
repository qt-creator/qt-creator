/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#ifndef IOSANALYZESUPPORT_H
#define IOSANALYZESUPPORT_H

#include "iosrunconfiguration.h"

#include <qmldebug/qmloutputparser.h>

#include <QProcess>
#include <QObject>

namespace Analyzer { class AnalyzerRunControl; }

namespace Ios {
namespace Internal {

class IosRunConfiguration;
class IosRunner;

class IosAnalyzeSupport : public QObject
{
    Q_OBJECT

public:
    static ProjectExplorer::RunControl *createAnalyzeRunControl(IosRunConfiguration *runConfig,
                                                                QString *errorMessage);

    IosAnalyzeSupport(IosRunConfiguration *runConfig,
                      Analyzer::AnalyzerRunControl *runControl, bool cppDebug, bool qmlDebug);
    ~IosAnalyzeSupport();

private:
    void qmlServerReady();
    void handleServerPorts(int gdbServerFd, int qmlPort);
    void handleGotInferiorPid(qint64 pid, int qmlPort);
    void handleRemoteProcessFinished(bool cleanEnd);

    void handleRemoteOutput(const QString &output);
    void handleRemoteErrorOutput(const QString &output);

    Analyzer::AnalyzerRunControl *m_runControl;
    IosRunner * const m_runner;
    QmlDebug::QmlOutputParser m_outputParser;
    int m_qmlPort;
};

} // namespace Internal
} // namespace Ios

#endif // IOSANALYZESUPPORT_H
