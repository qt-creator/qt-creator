/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef LOCALQMLPROFILERRUNNER_H
#define LOCALQMLPROFILERRUNNER_H

#include "abstractqmlprofilerrunner.h"

#include <utils/environment.h>
#include <projectexplorer/applicationlauncher.h>

namespace QmlProfiler {
namespace Internal {

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

    explicit LocalQmlProfilerRunner(const Configuration &configuration, QObject *parent = 0);

    // AbstractQmlProfilerRunner
    virtual void start();
    virtual void stop();
    virtual quint16 debugPort() const;

    bool hasExecutable() const { return !m_configuration.executable.isEmpty(); }

private slots:
    void spontaneousStop(int exitCode);

private:
    Configuration m_configuration;
    ProjectExplorer::ApplicationLauncher m_launcher;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // LOCALQMLPROFILERRUNNER_H
