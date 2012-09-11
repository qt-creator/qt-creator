/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (C) 2011 Kläralvdalens Datakonsult AB, a KDAB Group company.
**
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
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
#include "analyzersettings.h"
#include "analyzerruncontrol.h"
#include "analyzerrunconfigwidget.h"
#include "analyzermanager.h"
#include "ianalyzertool.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/applicationrunconfiguration.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/qtcassert.h>

#include <QAction>

using namespace ProjectExplorer;

namespace Analyzer {
namespace Internal {

AnalyzerRunControlFactory::AnalyzerRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

QString AnalyzerRunControlFactory::displayName() const
{
    return tr("Analyzer");
}

bool AnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    IAnalyzerTool *tool = AnalyzerManager::toolFromRunMode(mode);
    if (tool)
        return tool->canRun(runConfiguration, mode);
    return false;
}

RunControl *AnalyzerRunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
{
    IAnalyzerTool *tool = AnalyzerManager::toolFromRunMode(mode);
    if (!tool) {
        if (errorMessage)
            *errorMessage = tr("No analyzer tool selected"); // never happens
        return 0;
    }

    QTC_ASSERT(canRun(runConfiguration, mode), return 0);

    AnalyzerStartParameters sp = tool->createStartParameters(runConfiguration, mode);
    sp.toolId = tool->id();

    AnalyzerRunControl *rc = new AnalyzerRunControl(tool, sp, runConfiguration);
    QObject::connect(AnalyzerManager::stopAction(), SIGNAL(triggered()), rc, SLOT(stopIt()));
    return rc;
}

IRunConfigurationAspect *AnalyzerRunControlFactory::createRunConfigurationAspect()
{
    return new AnalyzerRunConfigurationAspect;
}

RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}

} // namespace Internal
} // namespace Analyzer
