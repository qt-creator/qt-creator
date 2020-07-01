/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
