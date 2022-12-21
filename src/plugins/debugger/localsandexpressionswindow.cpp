// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "localsandexpressionswindow.h"

#include <coreplugin/minisplitter.h>

#include <QVBoxLayout>
#include <QStackedWidget>

namespace Debugger::Internal {

enum { LocalsIndex = 0, InspectorIndex = 1 };

LocalsAndInspectorWindow::LocalsAndInspectorWindow(QWidget *locals,
      QWidget *inspector, QWidget *returnWidget)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto splitter = new Core::MiniSplitter(Qt::Vertical);
    layout->addWidget(splitter);

    auto localsAndInspector = new QStackedWidget;
    localsAndInspector->addWidget(locals);
    localsAndInspector->addWidget(inspector);
    localsAndInspector->setCurrentWidget(inspector);

    splitter->addWidget(localsAndInspector);
    splitter->addWidget(returnWidget);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(2, 1);
    splitter->setStretchFactor(3, 1);

    // Timer is to prevent flicker when switching between Inpector and Locals
    // when debugger engine changes states.
    m_timer.setSingleShot(true);
    m_timer.setInterval(500); // TODO: remove the magic number!
    connect(&m_timer, &QTimer::timeout, this, [this, localsAndInspector] {
        localsAndInspector->setCurrentIndex(m_showLocals ? LocalsIndex : InspectorIndex);
    });
}

void LocalsAndInspectorWindow::setShowLocals(bool showLocals)
{
    m_showLocals = showLocals;
    m_timer.start();
}

} // Debugger::Internal
