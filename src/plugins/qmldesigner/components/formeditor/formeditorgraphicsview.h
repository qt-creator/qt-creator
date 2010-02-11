/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FORMEDITORGRAPHICSVIEW_H
#define FORMEDITORGRAPHICSVIEW_H

#include <QGraphicsView>
#include <qmlitemnode.h>

namespace QmlDesigner {

class FormEditorGraphicsView : public QGraphicsView
{
Q_OBJECT
public:
    explicit FormEditorGraphicsView(QWidget *parent = 0);

    void setFeedbackNode(const QmlItemNode &node);

protected:
    void drawForeground(QPainter *painter, const QRectF &rect );
    void drawBackground(QPainter *painter, const QRectF &rect);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
private:
    QmlItemNode m_feedbackNode;
    QmlObjectNode m_parentNode;
    QVariant m_beginX;
    QVariant m_beginY;
    QVariant m_beginWidth;
    QVariant m_beginHeight;
    QVariant m_beginLeftMargin;
    QVariant m_beginRightMargin;
    QVariant m_beginTopMargin;
    QVariant m_beginBottomMargin;
    bool m_beginXHasExpression;
    bool m_beginYHasExpression;
    bool m_beginWidthHasExpression;
    bool m_beginHeightHasExpression;
    QPoint m_feedbackOriginPoint;
};

} // namespace QmlDesigner

#endif // FORMEDITORGRAPHICSVIEW_H
