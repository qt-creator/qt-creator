/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localsandexpressionswindow.h"

#include <coreplugin/minisplitter.h>

#include <QVBoxLayout>
#include <QStackedWidget>

namespace Debugger {
namespace Internal {

enum { LocalsIndex = 0, InspectorIndex = 1 };

LocalsAndExpressionsWindow::LocalsAndExpressionsWindow(QWidget *locals,
      QWidget *inspector, QWidget *returnWidget, QWidget *watchers)
    : m_showLocals(false)
{
    auto layout = new QVBoxLayout(this);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_splitter = new Core::MiniSplitter(Qt::Vertical);
    layout->addWidget(m_splitter);

    m_localsAndInspector = new QStackedWidget();
    m_localsAndInspector->addWidget(locals);
    m_localsAndInspector->addWidget(inspector);
    m_localsAndInspector->setCurrentWidget(inspector);

    m_splitter->addWidget(m_localsAndInspector);
    m_splitter->addWidget(returnWidget);
    m_splitter->addWidget(watchers);

    m_splitter->setStretchFactor(0, 3);
    m_splitter->setStretchFactor(2, 1);
    m_splitter->setStretchFactor(3, 1);

    // Timer is to prevent flicker when switching between Inpector and Locals
    // when debugger engine changes states.
    m_timer.setSingleShot(true);
    m_timer.setInterval(500); // TODO: remove the magic number!
    connect(&m_timer, &QTimer::timeout, [this] {
        m_localsAndInspector->setCurrentIndex(m_showLocals ? LocalsIndex : InspectorIndex);
    });
}

void LocalsAndExpressionsWindow::setShowLocals(bool showLocals)
{
    m_showLocals = showLocals;
    m_timer.start();
}

} // namespace Internal
} // namespace Debugger
