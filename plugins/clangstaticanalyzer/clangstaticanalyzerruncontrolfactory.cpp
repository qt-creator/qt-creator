/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com <http://qt.digia.com/>
**
** This file is part of the Qt Enterprise LicenseChecker Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com <http://qt.digia.com/>
**
****************************************************************************/

#include "clangstaticanalyzerruncontrolfactory.h"

#include <analyzerbase/analyzermanager.h>
#include <analyzerbase/analyzerruncontrol.h>
#include <analyzerbase/analyzerstartparameters.h>

#include <projectexplorer/localapplicationrunconfiguration.h>

using namespace Analyzer;
using namespace ProjectExplorer;

namespace ClangStaticAnalyzer {
namespace Internal {

ClangStaticAnalyzerRunControlFactory::ClangStaticAnalyzerRunControlFactory(QObject *parent)
    : IRunControlFactory(parent)
{
}

bool ClangStaticAnalyzerRunControlFactory::canRun(RunConfiguration *runConfiguration,
                                                  RunMode runMode) const
{
    return runMode == ClangStaticAnalyzerMode
            && (qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration));
}

RunControl *ClangStaticAnalyzerRunControlFactory::create(RunConfiguration *runConfiguration,
                                                         RunMode runMode,
                                                         QString *errorMessage)
{
    Q_UNUSED(errorMessage);
    Q_UNUSED(runMode);

    auto *rc = qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    QTC_ASSERT(rc, return 0);

    AnalyzerStartParameters sp;
    sp.runMode = runMode;
    sp.startMode = StartLocal;
    return AnalyzerManager::createRunControl(sp, runConfiguration);
}

} // namespace Internal
} // namespace ClangStaticAnalyzer
