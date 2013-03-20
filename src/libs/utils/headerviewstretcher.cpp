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

#include "headerviewstretcher.h"
#include <QTreeWidget>
#include <QHideEvent>
#include <QHeaderView>

using namespace Utils;

/*!
    \class Utils::HeaderViewStretcher

    \brief The class fixes QHeaderView to resize all columns to contents, except one
    stretching column.

    As opposed to standard QTreeWidget, all columns are
    still interactively resizable.
*/

HeaderViewStretcher::HeaderViewStretcher(QHeaderView *headerView, int columnToStretch)
        : QObject(headerView), m_columnToStretch(columnToStretch)
{
    headerView->installEventFilter(this);
    QHideEvent fake;
    HeaderViewStretcher::eventFilter(headerView, &fake);
}

bool HeaderViewStretcher::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == parent()) {
        if (ev->type() == QEvent::Show) {
            QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setResizeMode(i, QHeaderView::Interactive);
        } else if (ev->type() == QEvent::Hide) {
            QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
            for (int i = 0; i < hv->count(); ++i)
                hv->setResizeMode(i, i == m_columnToStretch ? QHeaderView::Stretch : QHeaderView::ResizeToContents);
        } else if (ev->type() == QEvent::Resize) {
            QHeaderView *hv = qobject_cast<QHeaderView*>(obj);
            if (hv->resizeMode(m_columnToStretch) == QHeaderView::Interactive) {
                QResizeEvent *re = static_cast<QResizeEvent*>(ev);
                int diff = re->size().width() - re->oldSize().width() ;
                hv->resizeSection(m_columnToStretch, qMax(32, hv->sectionSize(m_columnToStretch) + diff));
            }
        }
    }
    return false;
}
