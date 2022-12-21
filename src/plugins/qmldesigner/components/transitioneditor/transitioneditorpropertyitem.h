// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "transitioneditorsectionitem.h"

#include <modelnode.h>

#include <QGraphicsRectItem>

QT_FORWARD_DECLARE_CLASS(QLineEdit)

namespace QmlDesigner {

class TransitionEditorGraphicsScene;

class TransitionEditorPropertyItem : public TimelineItem
{
    Q_OBJECT

public:
    enum { Type = TransitionEditorConstants::transitionEditorPropertyItemUserType };

    static TransitionEditorPropertyItem *create(const ModelNode &animation,
                                        TransitionEditorSectionItem *parent = nullptr);
    int type() const override;
    void updateData();
    void updateParentData();

    bool isSelected() const;
    QString propertyName() const;
    void invalidateBar();
    AbstractView *view() const;
    ModelNode propertyAnimation() const;
    ModelNode pauseAnimation() const;
    void select();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;

private:
    TransitionEditorPropertyItem(TransitionEditorSectionItem *parent = nullptr);
    TransitionEditorGraphicsScene *transitionEditorGraphicsScene() const;

    ModelNode m_animation;
    TransitionEditorBarItem *m_barItem = nullptr;
};

} // namespace QmlDesigner
