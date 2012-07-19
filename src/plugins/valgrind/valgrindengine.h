/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef VALGRINDENGINE_H
#define VALGRINDENGINE_H

#include <analyzerbase/ianalyzerengine.h>

#include <utils/environment.h>

#include <valgrind/valgrindrunner.h>

#include <QString>
#include <QByteArray>
#include <QFutureInterface>
#include <QFutureWatcher>

namespace Analyzer {
class AnalyzerSettings;
}

namespace Valgrind {
namespace Internal {

class ValgrindEngine : public Analyzer::IAnalyzerEngine
{
    Q_OBJECT

public:
    ValgrindEngine(Analyzer::IAnalyzerTool *tool,
        const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);
    ~ValgrindEngine();

    bool start();
    void stop();

    QString executable() const;

protected:
    virtual QString progressTitle() const = 0;
    virtual QStringList toolArguments() const = 0;
    virtual Valgrind::ValgrindRunner *runner() = 0;

    Analyzer::AnalyzerSettings *m_settings;
    QFutureInterface<void> *m_progress;
    QFutureWatcher<void> *m_progressWatcher;

private slots:
    void handleProgressCanceled();
    void handleProgressFinished();
    void runnerFinished();

    void receiveProcessOutput(const QByteArray &, Utils::OutputFormat);
    void receiveProcessError(const QString &, QProcess::ProcessError);

private:
    bool m_isStopping;
};

} // namespace Internal
} // namespace Valgrind

#endif // VALGRINDENGINE_H
