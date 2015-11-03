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

#ifndef QMT_GRAPHICSCLASSITEM_H
#define QMT_GRAPHICSCLASSITEM_H

#include "objectitem.h"

#include "qmt/diagram_scene/capabilities/relationable.h"

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QGraphicsLineItem;
class QGraphicsTextItem;
QT_END_NAMESPACE


namespace qmt {

class DiagramSceneModel;
class DClass;
class CustomIconItem;
class ContextLabelItem;
class TemplateParameterBox;
class RelationStarter;
class Style;

class ClassItem :
        public ObjectItem,
        public IRelationable
{
public:
    ClassItem(DClass *klass, DiagramSceneModel *diagram_scene_model, QGraphicsItem *parent = 0);

    ~ClassItem();

public:

    void update();

public:

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersection_point, QLineF *intersection_line) const;

public:

    QSizeF getMinimumSize() const;

public:

    QPointF getRelationStartPos() const;

    void relationDrawn(const QString &id, const QPointF &to_scene_pos, const QList<QPointF> &intermediate_points);

protected:

    bool extendContextMenu(QMenu *menu);

    bool handleSelectedContextMenuAction(QAction *action);

private:

    QSizeF calcMinimumGeometry() const;

    void updateGeometry();

    void updateMembers(const Style *style);

private:

    CustomIconItem *m_customIcon;

    QGraphicsRectItem *m_shape;

    QGraphicsSimpleTextItem *m_namespace;

    QGraphicsSimpleTextItem *m_className;

    ContextLabelItem *m_contextLabel;

    QGraphicsLineItem *m_attributesSeparator;

    QString m_attributesText;

    QGraphicsTextItem *m_attributes;

    QGraphicsLineItem *m_methodsSeparator;

    QString m_methodsText;

    QGraphicsTextItem *m_methods;

    TemplateParameterBox *m_templateParameterBox;

    RelationStarter *m_relationStarter;

};

}

#endif // QMT_GRAPHICSCLASSITEM_H
