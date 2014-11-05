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

#ifndef CLANGSTATICANALYZERRUNCONTROLFACTORY_H
#define CLANGSTATICANALYZERRUNCONTROLFACTORY_H

#include "clangstaticanalyzertool.h"

#include <projectexplorer/runconfiguration.h>

namespace ClangStaticAnalyzer {
namespace Internal {

class ClangStaticAnalyzerRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    explicit ClangStaticAnalyzerRunControlFactory(ClangStaticAnalyzerTool *tool,
                                                  QObject *parent = 0);

    bool canRun(ProjectExplorer::RunConfiguration *runConfiguration,
                ProjectExplorer::RunMode runMode) const;

    ProjectExplorer::RunControl *create(ProjectExplorer::RunConfiguration *runConfiguration,
                                        ProjectExplorer::RunMode runMode,
                                        QString *errorMessage);

private:
    ClangStaticAnalyzerTool *m_tool;
};

} // namespace Internal
} // namespace ClangStaticAnalyzer

#endif // CLANGSTATICANALYZERRUNCONTROLFACTORY_H
