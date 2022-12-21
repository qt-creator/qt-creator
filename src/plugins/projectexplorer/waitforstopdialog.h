// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QDialog>
#include <QElapsedTimer>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {

class RunControl;

namespace Internal {

class WaitForStopDialog : public QDialog
{
    Q_OBJECT
public:
    explicit WaitForStopDialog(const QList<RunControl *> &runControls);

    bool canceled();
private:
    void updateProgressText();
    void runControlFinished(const RunControl *runControl);

    QList<ProjectExplorer::RunControl *> m_runControls;
    QLabel *m_progressLabel;
    QElapsedTimer m_timer;
};

} // namespace Internal
} // namespace ProjectExplorer
