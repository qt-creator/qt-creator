/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativetiledcanvas_p.h"

#include "qdeclarativecontext2d_p.h"

#include <QtGui/qpixmap.h>
#include <QtGui/qpainter.h>

TiledCanvas::TiledCanvas()
: m_context2d(new Context2D(this)), m_canvasSize(-1, -1), m_tileSize(100, 100)
{
    setFlag(QGraphicsItem::ItemHasNoContents, false);
    setAcceptedMouseButtons(Qt::LeftButton);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);
}

QSizeF TiledCanvas::canvasSize() const
{
    return m_canvasSize;
}

void TiledCanvas::setCanvasSize(const QSizeF &v)
{
    if (m_canvasSize != v) {
        m_canvasSize = v;
        emit canvasSizeChanged();
        update();
    }
}

QSize TiledCanvas::tileSize() const
{
    return m_tileSize;
}

void TiledCanvas::setTileSize(const QSize &v)
{
    if (v != m_tileSize) {
        m_tileSize = v;
        emit tileSizeChanged();
        update();
    }
}

QRectF TiledCanvas::canvasWindow() const
{
    return m_canvasWindow;
}

void TiledCanvas::setCanvasWindow(const QRectF &v)
{
    if (m_canvasWindow != v) {
        m_canvasWindow = v;
        emit canvasWindowChanged();
        update();
    }
}

void TiledCanvas::requestPaint()
{
    update();
}

void TiledCanvas::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_context2d->size() != m_tileSize)
        m_context2d->setSize(m_tileSize);

    const int tw = m_tileSize.width();
    const int th = m_tileSize.height();

    int h1 = m_canvasWindow.left() / tw;
    int htiles = ((m_canvasWindow.right() - h1 * tw) + tw - 1) / tw;

    int v1 = m_canvasWindow.top() / th;
    int vtiles = ((m_canvasWindow.bottom() - v1 * th) + th - 1) / th;

    for (int yy = 0; yy < vtiles; ++yy) {
        for (int xx = 0; xx < htiles; ++xx) {
            int ht = xx + h1;
            int vt = yy + v1;

            m_context2d->reset();
            m_context2d->setPainterTranslate(QPoint(-ht * tw, -vt * th));

            emit drawRegion(m_context2d, QRect(ht * tw, vt * th, tw, th));

            p->drawPixmap(-m_canvasWindow.x() + ht * tw, -m_canvasWindow.y() + vt * th, m_context2d->pixmap());
        }
    }
}

void TiledCanvas::componentComplete()
{
    const QMetaObject *metaObject = this->metaObject();
    int propertyCount = metaObject->propertyCount();
    int requestPaintMethod = metaObject->indexOfMethod("requestPaint()");
    for (int ii = TiledCanvas::staticMetaObject.propertyCount(); ii < propertyCount; ++ii) {
        QMetaProperty p = metaObject->property(ii);
        if (p.hasNotifySignal())
            QMetaObject::connect(this, p.notifySignalIndex(), this, requestPaintMethod, 0, 0);
    }
    QDeclarativeItem::componentComplete();
}


void TiledCanvas::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);
    qWarning("MPE");
}

QPixmap TiledCanvas::getTile(int xx, int yy)
{
    QPixmap pix(m_tileSize);

    pix.fill(Qt::green);

    QString text = QString::number(xx) + QLatin1Char(' ') + QString::number(yy);

    QPainter p(&pix);
    p.drawText(pix.rect(), Qt::AlignHCenter | Qt::AlignVCenter, text);

    return pix;
}

