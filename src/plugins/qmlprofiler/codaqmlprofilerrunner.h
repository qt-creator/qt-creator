/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef CODAQMLPROFILERRUNNER_H
#define CODAQMLPROFILERRUNNER_H

#include "abstractqmlprofilerrunner.h"

#include <utils/environment.h>
#include <projectexplorer/runconfiguration.h>
#include <qt4projectmanager/qt-s60/s60devicerunconfiguration.h>

namespace QmlProfiler {
namespace Internal {

class CodaQmlProfilerRunner : public AbstractQmlProfilerRunner
{
    Q_OBJECT
    Q_DISABLE_COPY(CodaQmlProfilerRunner)

    using AbstractQmlProfilerRunner::appendMessage; // don't hide signal
public:
    explicit CodaQmlProfilerRunner(Qt4ProjectManager::S60DeviceRunConfiguration *configuration,
                                   QObject *parent = 0);
    ~CodaQmlProfilerRunner();

    // AbstractQmlProfilerRunner
    virtual void start();
    virtual void stop();
    virtual int debugPort() const;

private slots:
    void appendMessage(ProjectExplorer::RunControl *, const QString &message,
                       Utils::OutputFormat format);
private:
    Qt4ProjectManager::S60DeviceRunConfiguration *m_configuration;
    ProjectExplorer::RunControl *m_runControl;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // CODAQMLPROFILERRUNNER_H
