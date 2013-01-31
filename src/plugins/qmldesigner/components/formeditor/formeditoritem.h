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

#ifndef FORMEDITORITEM_H
#define FORMEDITORITEM_H

#include <QWeakPointer>
#include <QGraphicsWidget>
#include <qmlitemnode.h>
#include "snappinglinecreator.h"

QT_BEGIN_NAMESPACE
class QTimeLine;
QT_END_NAMESPACE

namespace QmlDesigner {

class FormEditorScene;
class FormEditorView;
class AbstractFormEditorTool;

namespace Internal {
    class GraphicItemResizer;
    class MoveController;
}

class FormEditorItem : public QGraphicsObject
{
    Q_OBJECT

    friend class QmlDesigner::FormEditorScene;
public:
    ~FormEditorItem();

    void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget = 0 );

    bool isContainer() const;
    QmlItemNode qmlItemNode() const;


    enum { Type = UserType + 0xfffa };

    int type() const;

    static FormEditorItem* fromQGraphicsItem(QGraphicsItem *graphicsItem);

    void updateSnappingLines(const QList<FormEditorItem*> &exceptionList,
                             FormEditorItem *transformationSpaceItem);

    SnapLineMap topSnappingLines() const;
    SnapLineMap bottomSnappingLines() const;
    SnapLineMap leftSnappingLines() const;
    SnapLineMap rightSnappingLines() const;
    SnapLineMap horizontalCenterSnappingLines() const;
    SnapLineMap verticalCenterSnappingLines() const;

    SnapLineMap topSnappingOffsets() const;
    SnapLineMap bottomSnappingOffsets() const;
    SnapLineMap leftSnappingOffsets() const;
    SnapLineMap rightSnappingOffsets() const;

    QList<FormEditorItem*> childFormEditorItems() const;
    FormEditorScene *scene() const;
    FormEditorItem *parentItem() const;

    QRectF boundingRect() const;

    void updateGeometry();
    void updateVisibilty();

    void showAttention();

    FormEditorView *formEditorView() const;

    void setHighlightBoundingRect(bool highlight);

    void setContentVisible(bool visible);
    bool isContentVisible() const;

    bool isFormEditorVisible() const;
    void setFormEditorVisible(bool isVisible);

protected:
    AbstractFormEditorTool* tool() const;
    void paintBoundingRect(QPainter *painter) const;
    void paintPlaceHolderForInvisbleItem(QPainter *painter) const;

private slots:
    void changeAttention(qreal value);

private: // functions
    FormEditorItem(const QmlItemNode &qmlItemNode, FormEditorScene* scene);
    void setup();
    void setAttentionScale(double scale);
    void setAttentionHighlight(double value);

private: // variables
    SnappingLineCreator m_snappingLineCreator;
    QmlItemNode m_qmlItemNode;
    QWeakPointer<QTimeLine> m_attentionTimeLine;
    QTransform m_attentionTransform; // make item larger in anchor mode
    QTransform m_inverseAttentionTransform;
    QRectF m_boundingRect;
    double m_borderWidth;
    bool m_highlightBoundingRect;
    bool m_isContentVisible;
    bool m_isFormEditorVisible;
};


inline int FormEditorItem::type() const
{
    return UserType + 0xfffa;
}

}

#endif // FORMEDITORITEM_H
