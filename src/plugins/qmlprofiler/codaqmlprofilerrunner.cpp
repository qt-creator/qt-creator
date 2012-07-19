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

#include "codaqmlprofilerrunner.h"
#include <utils/qtcassert.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <extensionsystem/pluginmanager.h>
#include <qt4projectmanager/qt-s60/s60deployconfiguration.h>
#include <projectexplorer/runconfiguration.h>
#include <analyzerbase/analyzerconstants.h>
#include <qt4projectmanager/qt-s60/codaruncontrol.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace QmlProfiler::Internal;

CodaQmlProfilerRunner::CodaQmlProfilerRunner(S60DeviceRunConfiguration *configuration,
                                             QObject *parent) :
    AbstractQmlProfilerRunner(parent),
    m_configuration(configuration),
    m_runControl(new CodaRunControl(configuration, QmlProfilerRunMode))
{
    connect(m_runControl, SIGNAL(finished()), this, SIGNAL(stopped()));
    connect(m_runControl,
            SIGNAL(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)),
            this, SLOT(appendMessage(ProjectExplorer::RunControl*,QString,Utils::OutputFormat)));
}

CodaQmlProfilerRunner::~CodaQmlProfilerRunner()
{
    delete m_runControl;
}

void CodaQmlProfilerRunner::start()
{
    QTC_ASSERT(m_runControl, return);
    m_runControl->start();
}

void CodaQmlProfilerRunner::stop()
{
    QTC_ASSERT(m_runControl, return);
    m_runControl->stop();
}

void CodaQmlProfilerRunner::appendMessage(ProjectExplorer::RunControl *, const QString &message,
                                          Utils::OutputFormat format)
{
    emit appendMessage(message, format);
}

quint16 QmlProfiler::Internal::CodaQmlProfilerRunner::debugPort() const
{
    return m_configuration->debuggerAspect()->qmlDebugServerPort();
}

