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

#ifndef QNXANALYZESUPPORT_H
#define QNXANALYZESUPPORT_H

#include "qnxabstractrunsupport.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/outputformat.h>
#include <qmldebug/qmloutputparser.h>

namespace Analyzer { class AnalyzerRunControl; }

namespace Qnx {
namespace Internal {

class QnxRunConfiguration;
class Slog2InfoRunner;

class QnxAnalyzeSupport : public QnxAbstractRunSupport
{
    Q_OBJECT
public:
    QnxAnalyzeSupport(QnxRunConfiguration *runConfig, Analyzer::AnalyzerRunControl *engine);

public slots:
    void handleProfilingFinished();

private slots:
    void handleAdapterSetupRequested();

    void handleRemoteProcessFinished(bool success);
    void handleProgressReport(const QString &progressOutput);
    void handleRemoteOutput(const QByteArray &output);
    void handleError(const QString &error);

    void showMessage(const QString &, Utils::OutputFormat);
    void printMissingWarning();

    void remoteIsRunning();

private:
    void startExecution();

    Analyzer::AnalyzerRunControl *m_runControl;
    QmlDebug::QmlOutputParser m_outputParser;
    int m_qmlPort;

    Slog2InfoRunner *m_slog2Info;
};

} // namespace Internal
} // namespace Qnx

#endif // QNXANALYZESUPPORT_H
