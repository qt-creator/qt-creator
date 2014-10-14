/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#ifndef LOCALQMLPROFILERRUNNER_H
#define LOCALQMLPROFILERRUNNER_H

#include "abstractqmlprofilerrunner.h"

#include <utils/environment.h>
#include <projectexplorer/applicationlauncher.h>

namespace ProjectExplorer { class RunConfiguration; }
namespace Analyzer { class AnalyzerStartParameters; }

namespace QmlProfiler {
namespace Internal {

class QmlProfilerRunControl;
class LocalQmlProfilerRunner : public AbstractQmlProfilerRunner
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

    static LocalQmlProfilerRunner *createLocalRunner(ProjectExplorer::RunConfiguration *runConfiguration,
                                                     const Analyzer::AnalyzerStartParameters &sp,
                                                     QString *errorMessage,
                                                     QmlProfilerRunControl *engine);

    ~LocalQmlProfilerRunner();

    // AbstractQmlProfilerRunner
    virtual void start();
    virtual void stop();
    virtual quint16 debugPort() const;

private slots:
    void spontaneousStop(int exitCode, QProcess::ExitStatus status);

private:
    LocalQmlProfilerRunner(const Configuration &configuration, QmlProfilerRunControl *engine);

private:
    Configuration m_configuration;
    ProjectExplorer::ApplicationLauncher m_launcher;
    QmlProfilerRunControl *m_engine;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // LOCALQMLPROFILERRUNNER_H
