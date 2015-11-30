/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMT_SHAPEPAINTVISITOR_H
#define QMT_SHAPEPAINTVISITOR_H

#include "shapevisitor.h"
#include "qmt/infrastructure/qmt_global.h"

#include <QPainter>
#include <QPointF>
#include <QSizeF>

namespace qmt {

class QMT_EXPORT ShapePaintVisitor : public ShapeConstVisitor
{
public:
    ShapePaintVisitor(QPainter *painter, const QPointF &scaledOrigin, const QSizeF &originalSize,
                      const QSizeF &baseSize, const QSizeF &size);

    void visitLine(const LineShape *shapeLine) override;
    void visitRect(const RectShape *shapeRect) override;
    void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) override;
    void visitCircle(const CircleShape *shapeCircle) override;
    void visitEllipse(const EllipseShape *shapeEllipse) override;
    void visitArc(const ArcShape *shapeArc) override;
    void visitPath(const PathShape *shapePath) override;

private:
    QPainter *m_painter;
    QPointF m_scaledOrigin;
    QSizeF m_originalSize;
    QSizeF m_baseSize;
    QSizeF m_size;
};

class QMT_EXPORT ShapeSizeVisitor : public ShapeConstVisitor
{
public:
    ShapeSizeVisitor(const QPointF &scaledOrigin, const QSizeF &originalSize,
                     const QSizeF &baseSize, const QSizeF &size);

    QRectF boundingRect() const { return m_boundingRect; }

    void visitLine(const LineShape *shapeLine) override;
    void visitRect(const RectShape *shapeRect) override;
    void visitRoundedRect(const RoundedRectShape *shapeRoundedRect) override;
    void visitCircle(const CircleShape *shapeCircle) override;
    void visitEllipse(const EllipseShape *shapeEllipse) override;
    void visitArc(const ArcShape *shapeArc) override;
    void visitPath(const PathShape *shapePath) override;

private:
    QPointF m_scaledOrigin;
    QSizeF m_originalSize;
    QSizeF m_baseSize;
    QSizeF m_size;
    QRectF m_boundingRect;
};

} // namespace qmt

#endif // QMT_SHAPEPAINTVISITOR_H
