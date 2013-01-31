/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CALLGRINDENGINE_H
#define CALLGRINDENGINE_H

#include <valgrind/valgrindengine.h>

#include <valgrind/callgrind/callgrindrunner.h>
#include <valgrind/callgrind/callgrindparsedata.h>

namespace Valgrind {
namespace Internal {

class CallgrindEngine : public Valgrind::Internal::ValgrindEngine
{
    Q_OBJECT

public:
    CallgrindEngine(Analyzer::IAnalyzerTool *tool, const Analyzer::AnalyzerStartParameters &sp,
        ProjectExplorer::RunConfiguration *runConfiguration);

    bool start();

    Valgrind::Callgrind::ParseData *takeParserData();

    bool canPause() const { return true; }

public slots:
    /// controller actions
    void dump();
    void reset();
    void pause();
    void unpause();

    /// marks the callgrind process as paused
    /// calls pause() and unpause() if there's an active run
    void setPaused(bool paused);

    void setToggleCollectFunction(const QString &toggleCollectFunction);

protected:
    virtual QStringList toolArguments() const;
    virtual QString progressTitle() const;
    virtual Valgrind::ValgrindRunner *runner();

signals:
    void parserDataReady(CallgrindEngine *engine);

private slots:
    void slotFinished();
    void slotStarted();
    void showStatusMessage(const QString &msg);

private:
    Valgrind::Callgrind::CallgrindRunner m_runner;
    bool m_markAsPaused;

    QString m_argumentForToggleCollect;
};

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDENGINE_H
