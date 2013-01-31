/**************************************************************************
**
** Copyright (C) 2013 Kläralvdalens Datakonsult AB, a KDAB Group company.
** Contact: Kläralvdalens Datakonsult AB (info@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "analyzerruncontrolfactory.h"
#include "analyzersettings.h"
#include "analyzerruncontrol.h"
#include "analyzerrunconfigwidget.h"
#include "analyzermanager.h"
#include "ianalyzertool.h"
#include "analyzerstartparameters.h"

#include <projectexplorer/localapplicationrunconfiguration.h>
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

IRunConfigurationAspect *AnalyzerRunControlFactory::cloneRunConfigurationAspect(IRunConfigurationAspect *source)
{
    AnalyzerRunConfigurationAspect *s = dynamic_cast<AnalyzerRunConfigurationAspect *>(source);
    if (!s)
        return 0;
    return new AnalyzerRunConfigurationAspect(s);
}

RunConfigWidget *AnalyzerRunControlFactory::createConfigurationWidget(RunConfiguration *runConfiguration)
{
    AnalyzerRunConfigWidget *ret = new AnalyzerRunConfigWidget;
    ret->setRunConfiguration(runConfiguration);
    return ret;
}

} // namespace Internal
} // namespace Analyzer
