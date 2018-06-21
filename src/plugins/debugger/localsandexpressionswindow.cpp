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

#include "localsandexpressionswindow.h"

#include <coreplugin/minisplitter.h>

#include <QVBoxLayout>
#include <QStackedWidget>

namespace Debugger {
namespace Internal {

enum { LocalsIndex = 0, InspectorIndex = 1 };

LocalsAndInspectorWindow::LocalsAndInspectorWindow(QWidget *locals,
      QWidget *inspector, QWidget *returnWidget)
    : m_showLocals(false)
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
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
    connect(&m_timer, &QTimer::timeout, [this, localsAndInspector] {
        localsAndInspector->setCurrentIndex(m_showLocals ? LocalsIndex : InspectorIndex);
    });
}

void LocalsAndInspectorWindow::setShowLocals(bool showLocals)
{
    m_showLocals = showLocals;
    m_timer.start();
}

} // namespace Internal
} // namespace Debugger
