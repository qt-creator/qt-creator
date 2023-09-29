// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "todooutputtreeview.h"
#include "todooutputtreeviewdelegate.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>

#include <QResizeEvent>
#include <QHeaderView>

using namespace Utils;

namespace Todo {
namespace Internal {

TodoOutputTreeView::TodoOutputTreeView(QWidget *parent) :
    Utils::TreeView(parent)
{
    setRootIsDecorated(false);
    setFrameStyle(QFrame::NoFrame);
    setSortingEnabled(true);
    setAttribute(Qt::WA_MacShowFocusRect, false);
    setSelectionBehavior(QTreeView::SelectRows);
    setItemDelegate(new TodoOutputTreeViewDelegate(this));

    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(true);
    header()->setSectionsMovable(false);
    connect(header(), &QHeaderView::sectionResized, this, &TodoOutputTreeView::todoColumnResized);
    loadDisplaySettings();
}

TodoOutputTreeView::~TodoOutputTreeView()
{
    saveDisplaySettings();
}

void TodoOutputTreeView::saveDisplaySettings()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::SETTINGS_GROUP);
    settings->setValue(Constants::OUTPUT_PANE_TEXT_WIDTH,
                       columnWidth(Constants::OUTPUT_COLUMN_TEXT));
    settings->setValue(Constants::OUTPUT_PANE_FILE_WIDTH,
                       columnWidth(Constants::OUTPUT_COLUMN_FILE));
    settings->endGroup();
}

void TodoOutputTreeView::loadDisplaySettings()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::SETTINGS_GROUP);
    m_textColumnDefaultWidth = settings->value(Constants::OUTPUT_PANE_TEXT_WIDTH, 0).toInt();
    m_fileColumnDefaultWidth = settings->value(Constants::OUTPUT_PANE_FILE_WIDTH, 0).toInt();
    settings->endGroup();
}

void TodoOutputTreeView::resizeEvent(QResizeEvent *event)
{
    int widthText = m_textColumnDefaultWidth;
    int widthFile = m_fileColumnDefaultWidth;

    if ((event->oldSize().width() == 0) || (event->oldSize().width() == -1)) {
        if (widthText == 0)
            widthText = 0.55 * event->size().width();
        if (widthFile == 0)
            widthFile = 0.35 * event->size().width();
    } else {
        const qreal scale = static_cast<qreal>(event->size().width())
                / static_cast<qreal>(event->oldSize().width());
        widthText = scale * columnWidth(Constants::OUTPUT_COLUMN_TEXT);
        widthFile = scale * columnWidth(Constants::OUTPUT_COLUMN_FILE);
    }

    setColumnWidth(Constants::OUTPUT_COLUMN_TEXT, widthText);
    setColumnWidth(Constants::OUTPUT_COLUMN_FILE, widthFile);
}

void TodoOutputTreeView::todoColumnResized(int column, int oldSize, int newSize)
{
    Q_UNUSED(oldSize)
    if (column == Constants::OUTPUT_COLUMN_TEXT)
        m_textColumnDefaultWidth = newSize;
    else if (column == Constants::OUTPUT_COLUMN_FILE)
        m_fileColumnDefaultWidth = newSize;
}

} // namespace Internal
} // namespace Todo
