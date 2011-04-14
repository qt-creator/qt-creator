/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlprojectanalyzerruncontrolfactory.h"
#include "qmlprojectmanager/qmlprojectrunconfiguration.h"
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzersettings.h>
#include <analyzerbase/analyzerrunconfigwidget.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace ProjectExplorer;
using namespace QmlProfiler::Internal;

AnalyzerStartParameters localStartParameters(ProjectExplorer::RunConfiguration *runConfiguration)
{
    AnalyzerStartParameters sp;
    QTC_ASSERT(runConfiguration, return sp);
    QmlProjectManager::QmlProjectRunConfiguration *rc =
            qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return sp);

    sp.startMode = StartLocal;
    sp.environment = rc->environment();
    sp.workingDirectory = rc->workingDirectory();
    sp.debuggee = rc->observerPath();
    sp.debuggeeArgs = rc->viewerArguments();
    sp.displayName = rc->displayName();
    sp.connParams.host = QLatin1String("localhost");
    sp.connParams.port = rc->qmlDebugServerPort();
    return sp;
}

QmlProjectAnalyzerRunControlFactory::QmlProjectAnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{

}

QmlProjectAnalyzerRunControlFactory::~QmlProjectAnalyzerRunControlFactory()
{

}

bool QmlProjectAnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    if (!qobject_cast<QmlProjectManager::QmlProjectRunConfiguration *>(runConfiguration))
        return false;
    return mode == Constants::MODE_ANALYZE;
}

RunControl *QmlProjectAnalyzerRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    if (!qobject_cast<QmlProjectManager::QmlProjectRunConfiguration  *>(runConfiguration) ||
         mode != Constants::MODE_ANALYZE) {
        return 0;
    }
    const AnalyzerStartParameters sp = localStartParameters(runConfiguration);
    return create(sp, runConfiguration);
}

AnalyzerRunControl *QmlProjectAnalyzerRunControlFactory::create(const Analyzer::AnalyzerStartParameters &sp, RunConfiguration *runConfiguration)
{
    AnalyzerRunControl *rc = new AnalyzerRunControl(sp, runConfiguration);
    emit runControlCreated(rc);
    return rc;
}

QString QmlProjectAnalyzerRunControlFactory::displayName() const
{
    return tr("QmlAnalyzer");
}

IRunConfigurationAspect *QmlProjectAnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

RunConfigWidget *QmlProjectAnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    QmlProjectManager::QmlProjectRunConfiguration  *localRc =
        qobject_cast<QmlProjectManager::QmlProjectRunConfiguration  *>(runConfiguration);
    if (!localRc)
        return 0;

    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    Analyzer::AnalyzerRunConfigWidget *ret = new Analyzer::AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
//    return 0;
}
