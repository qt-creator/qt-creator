/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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
    void setRootItemRect(const QRectF &rect);
    QRectF rootItemRect() const;

protected:
    void drawForeground(QPainter *painter, const QRectF &rect );
    void drawBackground(QPainter *painter, const QRectF &rect);
    void wheelEvent(QWheelEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void leaveEvent(QEvent *);
    void keyPressEvent(QKeyEvent *event);


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
    QPixmap m_bubblePixmap;
    QRectF m_rootItemRect;
};

} // namespace QmlDesigner

#endif // FORMEDITORGRAPHICSVIEW_H
