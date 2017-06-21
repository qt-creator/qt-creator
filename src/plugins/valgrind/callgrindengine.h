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

#pragma once

#include "valgrindengine.h"
#include "valgrindrunner.h"

#include "callgrind/callgrindparsedata.h"
#include "callgrind/callgrindparser.h"
#include "callgrind/callgrindcontroller.h"

namespace Valgrind {
namespace Internal {

class CallgrindToolRunner : public ValgrindToolRunner
{
    Q_OBJECT

public:
    explicit CallgrindToolRunner(ProjectExplorer::RunControl *runControl);

    void start() override;

    Valgrind::Callgrind::ParseData *takeParserData();

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
    QStringList toolArguments() const override;
    QString progressTitle() const override;

signals:
    void parserDataReady(CallgrindToolRunner *engine);

private:
    void slotFinished();
    void showStatusMessage(const QString &message);

    void triggerParse();
    void localParseDataAvailable(const QString &file);
    void controllerFinished(Callgrind::CallgrindController::Option option);

    bool m_markAsPaused = false;
    Callgrind::CallgrindController m_controller;
    Callgrind::Parser m_parser;
    bool m_paused = false;

    QString m_argumentForToggleCollect;
};

} // namespace Internal
} // namespace Valgrind
