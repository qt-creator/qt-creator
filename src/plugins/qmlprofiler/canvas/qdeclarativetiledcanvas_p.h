/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVETILEDCANVAS_P_H
#define QDECLARATIVETILEDCANVAS_P_H

#include <QtDeclarative/qdeclarativeitem.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class Context2D;
class TiledCanvas : public QDeclarativeItem
{
    Q_OBJECT

    Q_PROPERTY(QSizeF canvasSize READ canvasSize WRITE setCanvasSize NOTIFY canvasSizeChanged)
    Q_PROPERTY(QSize tileSize READ tileSize WRITE setTileSize NOTIFY tileSizeChanged)
    Q_PROPERTY(QRectF canvasWindow READ canvasWindow WRITE setCanvasWindow NOTIFY canvasWindowChanged)

public:
    TiledCanvas();

    QSizeF canvasSize() const;
    void setCanvasSize(const QSizeF &);

    QSize tileSize() const;
    void setTileSize(const QSize &);

    QRectF canvasWindow() const;
    void setCanvasWindow(const QRectF &);

Q_SIGNALS:
    void canvasSizeChanged();
    void tileSizeChanged();
    void canvasWindowChanged();

    void drawRegion(Context2D *ctxt, const QRect &region);

public Q_SLOTS:
    void requestPaint();

protected:
    virtual void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    virtual void componentComplete();
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);

private:
    QPixmap getTile(int, int);

    Context2D *m_context2d;

    QSizeF m_canvasSize;
    QSize m_tileSize;
    QRectF m_canvasWindow;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QDECLARATIVETILEDCANVAS_P_H
