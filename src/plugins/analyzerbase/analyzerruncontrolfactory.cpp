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

#include "analyzerruncontrolfactory.h"
#include "analyzerruncontrol.h"
#include "analyzerconstants.h"
#include "analyzermanager.h"
#include "analyzerrunconfigwidget.h"
#include "analyzersettings.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/applicationrunconfiguration.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>

using namespace Analyzer;
using namespace Analyzer::Internal;
using namespace ProjectExplorer;

/////////////////////////////////////////////////////////////////////////////////
//
// AnalyzerRunControlFactory
//
/////////////////////////////////////////////////////////////////////////////////

AnalyzerRunControlFactory::AnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
    setObjectName(QLatin1String("AnalyzerRunControlFactory"));
}

bool AnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, const QString &mode) const
{
    return runConfiguration->isEnabled() && mode == Constants::MODE_ANALYZE;
}

RunControl *AnalyzerRunControlFactory::create(RunConfiguration *runConfiguration, const QString &mode)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    return AnalyzerManager::createRunControl(runConfiguration, mode);
}

QString AnalyzerRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

IRunConfigurationAspect *AnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerProjectSettings;
}

RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    LocalApplicationRunConfiguration *localRc =
        qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    if (!localRc)
        return 0;
    AnalyzerProjectSettings *settings = runConfiguration->extraAspect<AnalyzerProjectSettings>();
    if (!settings)
        return 0;

    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}
