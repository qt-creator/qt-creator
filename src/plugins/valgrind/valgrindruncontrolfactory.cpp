/****************************************************************************
**
** Copyright (C) 2013 Kläralvdalens Datakonsult AB, a KDAB Group company, info@kdab.com
** Contact: http://www.qt-project.org/legal
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

#include "valgrindruncontrolfactory.h"

#include <analyzerbase/ianalyzertool.h>
#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerstartparameters.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzersettings.h>

#include <utils/qtcassert.h>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace Valgrind {
namespace Internal {

ValgrindRunControlFactory::ValgrindRunControlFactory(QObject *parent) :
    IRunControlFactory(parent)
{
}

bool ValgrindRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    if (mode != CallgrindRunMode && mode != MemcheckRunMode)
        return false;
    IAnalyzerTool *tool = AnalyzerManager::toolFromRunMode(mode);
    if (tool)
        return tool->canRun(runConfiguration, mode);
    return false;
}

RunControl *ValgrindRunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
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
    return rc;
}

IRunConfigurationAspect *ValgrindRunControlFactory::createRunConfigurationAspect(RunConfiguration *rc)
{
    Q_UNUSED(rc);
    return new AnalyzerRunConfigurationAspect;
}

} // namespace Internal
} // namespace Valgrind
