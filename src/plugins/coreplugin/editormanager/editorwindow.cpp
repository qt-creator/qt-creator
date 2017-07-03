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

#include "editorwindow.h"

#include "editorarea.h"
#include "editormanager_p.h"

#include <aggregation/aggregate.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/locatormanager.h>
#include <coreplugin/minisplitter.h>

#include <QStatusBar>
#include <QVBoxLayout>

namespace Core {
namespace Internal {

EditorWindow::EditorWindow(QWidget *parent) :
    QWidget(parent)
{
    m_area = new EditorArea;
    auto layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    setLayout(layout);
    layout->addWidget(m_area);
    setFocusProxy(m_area);
    auto statusBar = new QStatusBar;
    layout->addWidget(statusBar);
    auto splitter = new NonResizingSplitter(statusBar);
    splitter->setChildrenCollapsible(false);
    statusBar->addPermanentWidget(splitter, 10);
    auto locatorWidget = LocatorManager::createLocatorInputWidget(this);
    splitter->addWidget(locatorWidget);
    splitter->addWidget(new QWidget);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_QuitOnClose, false); // don't prevent Qt Creator from closing
    resize(QSize(800, 600));

    static int windowId = 0;
    ICore::registerWindow(this, Context(Id("EditorManager.ExternalWindow.").withSuffix(++windowId)));

    connect(m_area, &EditorArea::windowTitleNeedsUpdate,
            this, &EditorWindow::updateWindowTitle);
    // editor area can be deleted by editor manager
    connect(m_area, &EditorArea::destroyed, this, [this]() {
        m_area = nullptr;
        deleteLater();
    });
    updateWindowTitle();
}

EditorWindow::~EditorWindow()
{
    if (m_area)
        disconnect(m_area, 0, this, 0);
}

EditorArea *EditorWindow::editorArea() const
{
    return m_area;
}

void EditorWindow::updateWindowTitle()
{
    EditorManagerPrivate::updateWindowTitleForDocument(m_area->currentDocument(), this);
}

} // Internal
} // Core
