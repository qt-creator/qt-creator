// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "objectitem.h"

#include "qmt/diagram/dclass.h"

QT_BEGIN_NAMESPACE
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
class QGraphicsLineItem;
class QGraphicsTextItem;
QT_END_NAMESPACE

namespace qmt {

class DiagramSceneModel;
class CustomIconItem;
class ContextLabelItem;
class TemplateParameterBox;
class Style;

class ClassItem : public ObjectItem
{
public:
    ClassItem(DClass *klass, DiagramSceneModel *diagramSceneModel, QGraphicsItem *parent = nullptr);
    ~ClassItem() override;

    void update() override;

    bool intersectShapeWithLine(const QLineF &line, QPointF *intersectionPoint,
                                QLineF *intersectionLine) const override;

    QSizeF minimumSize() const override;

    void relationDrawn(const QString &id, ObjectItem *targetElement,
                       const QList<QPointF> &intermediatePoints) override;

protected:
    bool extendContextMenu(QMenu *menu) override;
    bool handleSelectedContextMenuAction(const QString &id) override;
    QString buildDisplayName() const override;
    void setFromDisplayName(const QString &displayName) override;
    void addRelationStarterTool(const QString &id) override;
    void addRelationStarterTool(const CustomRelation &customRelation) override;
    void addStandardRelationStarterTools() override;

private:
    DClass::TemplateDisplay templateDisplay() const;
    QSizeF calcMinimumGeometry() const;
    void updateGeometry();
    void updateMembers(const Style *style);

    CustomIconItem *m_customIcon = nullptr;
    QGraphicsRectItem *m_shape = nullptr;
    QGraphicsSimpleTextItem *m_baseClasses = nullptr;
    QGraphicsSimpleTextItem *m_namespace = nullptr;
    ContextLabelItem *m_contextLabel = nullptr;
    QGraphicsLineItem *m_attributesSeparator = nullptr;
    QString m_attributesText;
    QGraphicsTextItem *m_attributes = nullptr;
    QGraphicsLineItem *m_methodsSeparator = nullptr;
    QString m_methodsText;
    QGraphicsTextItem *m_methods = nullptr;
    TemplateParameterBox *m_templateParameterBox = nullptr;
};

} // namespace qmt
