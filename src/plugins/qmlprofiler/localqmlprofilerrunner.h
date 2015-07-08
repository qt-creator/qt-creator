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

#ifndef LOCALQMLPROFILERRUNNER_H
#define LOCALQMLPROFILERRUNNER_H

#include "qmlprofiler_global.h"
#include <utils/environment.h>
#include <projectexplorer/applicationlauncher.h>

namespace ProjectExplorer { class RunConfiguration; }
namespace Analyzer {
    class AnalyzerStartParameters;
    class AnalyzerRunControl;
}

namespace QmlProfiler {

class QmlProfilerRunControl;
class QMLPROFILER_EXPORT LocalQmlProfilerRunner : public QObject
{
    Q_OBJECT

public:
    struct Configuration {
        QString executable;
        QString executableArguments;
        quint16 port;
        QString workingDirectory;
        Utils::Environment environment;
    };

    static Analyzer::AnalyzerRunControl *createLocalRunControl(
            ProjectExplorer::RunConfiguration *runConfiguration,
            const Analyzer::AnalyzerStartParameters &sp,
            QString *errorMessage);

    static quint16 findFreePort(QString &host);

    ~LocalQmlProfilerRunner();

signals:
    void started();
    void stopped();
    void appendMessage(const QString &message, Utils::OutputFormat format);

private slots:
    void spontaneousStop(int exitCode, QProcess::ExitStatus status);
    void start();
    void stop();

private:
    LocalQmlProfilerRunner(const Configuration &configuration, QmlProfilerRunControl *engine);

    Configuration m_configuration;
    ProjectExplorer::ApplicationLauncher m_launcher;
    QmlProfilerRunControl *m_engine;
};

} // namespace QmlProfiler

#endif // LOCALQMLPROFILERRUNNER_H
