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

#ifndef FORMEDITORITEM_H
#define FORMEDITORITEM_H

#include <QWeakPointer>
#include <QGraphicsWidget>
#include <qmlitemnode.h>
#include "snappinglinecreator.h"

class QTimeLine;

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

protected:
    AbstractFormEditorTool* tool() const;
    void paintBoundingRect(QPainter *painter) const;

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
    double m_opacity;
};


inline int FormEditorItem::type() const
{
    return UserType + 0xfffa;
}

}

#endif // FORMEDITORITEM_H
