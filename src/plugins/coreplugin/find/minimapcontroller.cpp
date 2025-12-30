// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "minimapcontroller.h"
#include "minimapoverlay.h"

#include <utils/plaintextedit/plaintextedit.h>

#include <QAbstractScrollArea>
#include <QScrollBar>

using namespace Utils;

namespace Core {

MinimapController::~MinimapController()
{
    if (m_minimap)
        delete m_minimap;
}

QScrollBar *MinimapController::scrollBar() const
{
    if (m_scrollArea)
        return m_scrollArea->verticalScrollBar();

    return nullptr;
}

QAbstractScrollArea *MinimapController::scrollArea() const
{
    return m_scrollArea;
}

void MinimapController::setScrollArea(QAbstractScrollArea *scrollArea)
{
    if (m_scrollArea == scrollArea)
        return;

    if (m_minimap) {
        delete m_minimap;
        m_minimap = nullptr;
    }

    m_scrollArea = scrollArea;

    if (m_scrollArea) {
        PlainTextEdit *editor = qobject_cast<PlainTextEdit *>(scrollArea);
        m_minimap = new MinimapOverlay(editor);
        m_minimap->scheduleUpdate();
    }
}

} // namespace Core
