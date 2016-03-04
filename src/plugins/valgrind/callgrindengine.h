/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#ifndef CALLGRINDENGINE_H
#define CALLGRINDENGINE_H

#include <valgrind/valgrindengine.h>

#include <valgrind/callgrind/callgrindrunner.h>
#include <valgrind/callgrind/callgrindparsedata.h>

namespace Valgrind {
namespace Internal {

class CallgrindRunControl : public ValgrindRunControl
{
    Q_OBJECT

public:
    CallgrindRunControl(ProjectExplorer::RunConfiguration *runConfiguration);

    bool startEngine() override;

    Valgrind::Callgrind::ParseData *takeParserData();

    bool canPause() const override { return true; }

public slots:
    /// controller actions
    void dump();
    void reset();
    void pause() override;
    void unpause() override;

    /// marks the callgrind process as paused
    /// calls pause() and unpause() if there's an active run
    void setPaused(bool paused);

    void setToggleCollectFunction(const QString &toggleCollectFunction);

protected:
    QStringList toolArguments() const override;
    QString progressTitle() const override;
    Valgrind::ValgrindRunner *runner() override;

signals:
    void parserDataReady(CallgrindRunControl *engine);

private:
    void slotFinished();

    Valgrind::Callgrind::CallgrindRunner m_runner;
    bool m_markAsPaused;

    QString m_argumentForToggleCollect;
};

} // namespace Internal
} // namespace Valgrind

#endif // CALLGRINDENGINE_H
