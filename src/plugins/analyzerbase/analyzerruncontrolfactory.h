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

#ifndef ANALYZERRUNCONTROLFACTORY_H
#define ANALYZERRUNCONTROLFACTORY_H

#include <projectexplorer/runconfiguration.h>

namespace Analyzer {

class AnalyzerRunControl;
class AnalyzerStartParameters;

namespace Internal {

class AnalyzerRunControlFactory: public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    AnalyzerRunControlFactory(QObject *parent = 0);

    typedef ProjectExplorer::RunConfiguration RunConfiguration;
    typedef ProjectExplorer::RunControl RunControl;

    // virtuals from IRunControlFactory
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    AnalyzerRunControl *create(const AnalyzerStartParameters &sp, RunConfiguration *rc = 0);
    QString displayName() const;

    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect();
    ProjectExplorer::RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

signals:
    void runControlCreated(Analyzer::AnalyzerRunControl *);
};

} // namespace Internal
} // namespace Analyzer

#endif // ANALYZERRUNCONTROLFACTORY_H
