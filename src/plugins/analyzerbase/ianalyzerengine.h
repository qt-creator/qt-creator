/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef IANALYZERENGINE_H
#define IANALYZERENGINE_H

#include "analyzerbase_global.h"

#include <projectexplorer/task.h>

#include <QtCore/QObject>
#include <QtCore/QString>

namespace ProjectExplorer {
class RunConfiguration;
}

namespace Analyzer {

class ANALYZER_EXPORT IAnalyzerEngine : public QObject
{
    Q_OBJECT
public:
    explicit IAnalyzerEngine(ProjectExplorer::RunConfiguration *runConfiguration);

    virtual void start() = 0;
    /// trigger async stop
    virtual void stop() = 0;

    ProjectExplorer::RunConfiguration *runConfiguration() const;

signals:
    void standardOutputReceived(const QString &);
    void standardErrorReceived(const QString &);
    void taskToBeAdded(ProjectExplorer::Task::TaskType type, const QString &description,
                       const QString &file, int line);
    void finished();
    void starting(const IAnalyzerEngine *);

private:
    ProjectExplorer::RunConfiguration *m_runConfig;
};

} // namespace Analyzer

#endif // IANALYZERENGINE_H
