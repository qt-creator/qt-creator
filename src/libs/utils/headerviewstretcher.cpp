// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "headerviewstretcher.h"

#include <QHideEvent>
#include <QHeaderView>

using namespace Utils;

/*!
    \class Utils::HeaderViewStretcher
    \inmodule QtCreator

    \brief The HeaderViewStretcher class fixes QHeaderView to resize all
    columns to contents, except one
    stretching column.

    As opposed to standard QTreeWidget, all columns are
    still interactively resizable.
*/

HeaderViewStretcher::HeaderViewStretcher(QHeaderView *headerView, int columnToStretch)
        : QObject(headerView), m_columnToStretch(columnToStretch)
{
    headerView->installEventFilter(this);
    stretch();
}

void HeaderViewStretcher::stretch()
{
    QHideEvent fake;
    HeaderViewStretcher::eventFilter(parent(), &fake);
}

void HeaderViewStretcher::softStretch()
{
    const auto hv = qobject_cast<QHeaderView*>(parent());
    for (int i = 0; i < hv->count(); ++i)
        hv->resizeSections(QHeaderView::ResizeToContents);
}

bool HeaderViewStretcher::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent()) {
        if (ev->type() == QEvent::Show) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setSectionResizeMode(i, QHeaderView::Interactive);
        } else if (ev->type() == QEvent::Hide) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setSectionResizeMode(i, i == m_columnToStretch
                                         ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
        } else if (ev->type() == QEvent::Resize) {
            auto hv = qobject_cast<QHeaderView*>(obj);
            if (hv->sectionResizeMode(m_columnToStretch) == QHeaderView::Interactive) {
                auto re = static_cast<QResizeEvent*>(ev);
                int diff = re->size().width() - re->oldSize().width() ;
                hv->resizeSection(m_columnToStretch, qMax(32, hv->sectionSize(m_columnToStretch) + diff));
            }
        }
    }
    return false;
}
