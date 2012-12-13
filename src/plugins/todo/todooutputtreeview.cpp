/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "todooutputtreeview.h"
#include "constants.h"

#include <coreplugin/icore.h>

#include <QResizeEvent>
#include <QHeaderView>
#include <QSettings>

namespace Todo {
namespace Internal {

TodoOutputTreeView::TodoOutputTreeView(QWidget *parent) :
    QTreeView(parent),
    m_textColumnDefaultWidth(0),
    m_fileColumnDefaultWidth(0)
{
    setRootIsDecorated(false);
    setFrameStyle(QFrame::NoFrame);
    setSortingEnabled(true);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    header()->setResizeMode(QHeaderView::Interactive);
    header()->setStretchLastSection(true);
    header()->setMovable(false);
    connect(header(), SIGNAL(sectionResized(int,int,int)), SLOT(todoColumnResized(int,int,int)));
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
