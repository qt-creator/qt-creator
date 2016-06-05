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

#include "todooutputtreeview.h"
#include "todooutputtreeviewdelegate.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <QResizeEvent>
#include <QHeaderView>
#include <QSettings>

namespace Todo {
namespace Internal {

TodoOutputTreeView::TodoOutputTreeView(QWidget *parent) :
    Utils::TreeView(parent),
    m_textColumnDefaultWidth(0),
    m_fileColumnDefaultWidth(0)
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
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    settings->setValue(QLatin1String(Constants::OUTPUT_PANE_TEXT_WIDTH),
                       columnWidth(Constants::OUTPUT_COLUMN_TEXT));
    settings->setValue(QLatin1String(Constants::OUTPUT_PANE_FILE_WIDTH),
                       columnWidth(Constants::OUTPUT_COLUMN_FILE));
    settings->endGroup();
}

void TodoOutputTreeView::loadDisplaySettings()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(QLatin1String(Constants::SETTINGS_GROUP));
    m_textColumnDefaultWidth = settings->value(
                QLatin1String(Constants::OUTPUT_PANE_TEXT_WIDTH), 0).toInt();
    m_fileColumnDefaultWidth = settings->value(
                QLatin1String(Constants::OUTPUT_PANE_FILE_WIDTH), 0).toInt();
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
    Q_UNUSED(oldSize);
    if (column == Constants::OUTPUT_COLUMN_TEXT)
        m_textColumnDefaultWidth = newSize;
    else if (column == Constants::OUTPUT_COLUMN_FILE)
        m_fileColumnDefaultWidth = newSize;
}

} // namespace Internal
} // namespace Todo
