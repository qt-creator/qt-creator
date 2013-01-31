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

#ifndef QDECLARATIVECANVAS_P_H
#define QDECLARATIVECANVAS_P_H

#include <qdeclarativeitem.h>

#include "qdeclarativecontext2d_p.h"
#include "qdeclarativecanvastimer_p.h"

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)
class Canvas : public QDeclarativeItem
{
    Q_OBJECT

    Q_ENUMS(FillMode)
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged);
    Q_PROPERTY(int canvasWidth READ canvasWidth WRITE setCanvasWidth NOTIFY canvasWidthChanged);
    Q_PROPERTY(int canvasHeight READ canvasHeight WRITE setCanvasHeight NOTIFY canvasHeightChanged);
    Q_PROPERTY(FillMode fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)

public:
    Canvas(QDeclarativeItem *parent = 0);
    enum FillMode { Stretch, PreserveAspectFit, PreserveAspectCrop, Tile, TileVertically, TileHorizontally };


    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
    void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
    void setCanvasWidth(int newWidth);
    int canvasWidth() {return m_canvasWidth;}

    void setCanvasHeight(int canvasHeight);
    int canvasHeight() {return m_canvasHeight;}

    void componentComplete();


public Q_SLOTS:
    Context2D *getContext(const QString & = QLatin1String("2d"));
    void requestPaint();

    FillMode fillMode() const;
    void setFillMode(FillMode);

    QColor color();
    void setColor(const QColor &);

    // Save current canvas to disk
    bool save(const QString& filename) const;

    // Timers
    void setInterval(const QScriptValue &handler, long timeout);
    void setTimeout(const QScriptValue &handler, long timeout);
    void clearInterval(const QScriptValue &handler);
    void clearTimeout(const QScriptValue &handler);

Q_SIGNALS:
    void fillModeChanged();
    void canvasWidthChanged();
    void canvasHeightChanged();
    void colorChanged();
    void init();
    void paint();

private:
    // Return canvas contents as a drawable image
    CanvasImage *toImage() const;
    Context2D *m_context;
    int m_canvasWidth;
    int m_canvasHeight;
    FillMode m_fillMode;
    QColor m_color;

    friend class Context2D;
};

QT_END_NAMESPACE

#endif //QDECLARATIVECANVAS_P_H
