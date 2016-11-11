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

#include <QObject>
#include <QFutureInterface>
#include <QProcess>
#include <QTimer>

namespace Core {
namespace Internal {

class CorePlugin;

class ProcessReaper : public QObject
{
    Q_OBJECT

public:
    ProcessReaper(QProcess *p, int timeoutMs);
    ~ProcessReaper();

    int timeoutMs() const;
    bool isFinished() const;
    void nextIteration();

private:
    mutable QTimer m_iterationTimer;
    QFutureInterface<void> m_futureInterface;
    QProcess *m_process;
    int m_emergencyCounter = 0;
    QProcess::ProcessState m_lastState = QProcess::NotRunning;
};

class ReaperPrivate {
public:
    ~ReaperPrivate();

    QList<ProcessReaper *> m_reapers;

private:
    ReaperPrivate();

    friend class CorePlugin;
};

} // namespace Internal
} // namespace Core
