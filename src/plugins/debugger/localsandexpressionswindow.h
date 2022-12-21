// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>
#include <QTimer>

namespace Debugger::Internal {

class LocalsAndInspectorWindow : public QWidget
{
public:
    LocalsAndInspectorWindow(QWidget *locals, QWidget *inspector,
                             QWidget *returnWidget); // TODO parent?

    void setShowLocals(bool showLocals);

private:
    QTimer m_timer;
    bool m_showLocals = false;
};

} // Debugger::Internal
