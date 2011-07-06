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

#ifndef QMLPROFILERRUNCONTROLFACTORY_H
#define QMLPROFILERRUNCONTROLFACTORY_H

#include <analyzerbase/analyzerruncontrol.h>
#include <projectexplorer/runconfiguration.h>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerRunControlFactory : public ProjectExplorer::IRunControlFactory
{
    Q_OBJECT

public:
    typedef ProjectExplorer::RunConfiguration RunConfiguration;

    QmlProfilerRunControlFactory(QObject *parent = 0);

    // IRunControlFactory implementation
    QString displayName() const;
    bool canRun(RunConfiguration *runConfiguration, const QString &mode) const;
    ProjectExplorer::RunControl *create(RunConfiguration *runConfiguration, const QString &mode);
    ProjectExplorer::IRunConfigurationAspect *createRunConfigurationAspect();
    ProjectExplorer::RunConfigWidget *createConfigurationWidget(RunConfiguration *runConfiguration);

signals:
    void runControlCreated(Analyzer::AnalyzerRunControl *);
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERRUNCONTROLFACTORY_H
