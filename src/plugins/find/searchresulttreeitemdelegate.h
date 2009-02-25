/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SEARCHRESULTTREEITEMDELEGATE_H
#define SEARCHRESULTTREEITEMDELEGATE_H

#include <QtGui/QItemDelegate>

namespace Find {
namespace Internal {

class SearchResultTreeItemDelegate: public QItemDelegate
{
public:
    SearchResultTreeItemDelegate(QObject *parent = 0);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

private:
    int drawLineNumber(QPainter *painter, const QStyleOptionViewItemV3 &option, const QModelIndex &index) const;
    void drawMarker(QPainter *painter, const QModelIndex &index, const QString text, const QRect &rect) const;

    static const int m_minimumLineNumberDigits = 6;
};

} // namespace Internal
} // namespace Find

#endif // SEARCHRESULTTREEITEMDELEGATE_H
