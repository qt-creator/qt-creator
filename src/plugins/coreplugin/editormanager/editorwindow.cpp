// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "editorwindow.h"

#include "editorarea.h"
#include "editormanager_p.h"
#include "../coreconstants.h"
#include "../icontext.h"
#include "../icore.h"
#include "../locator/locatormanager.h"
#include "../minisplitter.h"

#include <aggregation/aggregate.h>
#include <utils/qtcassert.h>

#include <QStatusBar>
#include <QVBoxLayout>

const char geometryKey[] = "geometry";
const char splitStateKey[] = "splitstate";

namespace Core {
namespace Internal {

EditorWindow::EditorWindow(QWidget *parent) :
    QWidget(parent)
{
    m_area = new EditorArea;
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
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

    ICore::registerWindow(this,
                          Context(Utils::Id("EditorManager.ExternalWindow.").withSuffix(++windowId),
                                  Constants::C_EDITORMANAGER));

    connect(m_area, &EditorArea::windowTitleNeedsUpdate,
            this, &EditorWindow::updateWindowTitle);
    // editor area can be deleted by editor manager
    connect(m_area, &EditorArea::destroyed, this, [this] {
        m_area = nullptr;
        deleteLater();
    });
    updateWindowTitle();
}

EditorWindow::~EditorWindow()
{
    if (m_area)
        disconnect(m_area, nullptr, this, nullptr);
}

EditorArea *EditorWindow::editorArea() const
{
    return m_area;
}

QVariantHash EditorWindow::saveState() const
{
    QVariantHash state;
    state.insert(geometryKey, saveGeometry());
    if (QTC_GUARD(m_area)) {
        const QByteArray splitState = m_area->saveState();
        state.insert(splitStateKey, splitState);
    }
    return state;
}

void EditorWindow::restoreState(const QVariantHash &state)
{
    if (state.contains(geometryKey))
        restoreGeometry(state.value(geometryKey).toByteArray());
    if (state.contains(splitStateKey) && m_area)
        m_area->restoreState(state.value(splitStateKey).toByteArray());
}

void EditorWindow::updateWindowTitle()
{
    EditorManagerPrivate::updateWindowTitleForDocument(m_area->currentDocument(), this);
}

} // Internal
} // Core
