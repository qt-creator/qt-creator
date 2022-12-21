// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "taskwindow.h"

#include <QPointer>
#include <QWidget>

QT_FORWARD_DECLARE_CLASS(QLabel)

namespace ProjectExplorer {
namespace Internal {

class BuildProgress : public QWidget
{
    Q_OBJECT

public:
    explicit BuildProgress(TaskWindow *taskWindow, Qt::Orientation orientation = Qt::Vertical);

private:
    void updateState();

    QWidget *m_contentWidget;
    QLabel *m_errorIcon;
    QLabel *m_warningIcon;
    QLabel *m_errorLabel;
    QLabel *m_warningLabel;
    QPointer<TaskWindow> m_taskWindow;
};

} // namespace Internal
} // namespace ProjectExplorer
