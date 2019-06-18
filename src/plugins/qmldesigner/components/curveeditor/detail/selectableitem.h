/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include <QGraphicsObject>

namespace DesignTools {

enum ItemType
{
    ItemTypeKeyframe = QGraphicsItem::UserType + 1,
    ItemTypeHandle = QGraphicsItem::UserType + 2,
    ItemTypeCurve = QGraphicsItem::UserType + 3
};

enum class SelectionMode : unsigned int
{
    Undefined,
    Clear,
    New,
    Add,
    Remove,
    Toggle
};

class SelectableItem : public QGraphicsObject
{
    Q_OBJECT

public:
    SelectableItem(QGraphicsItem *parent = nullptr);

    ~SelectableItem() override;

    bool activated() const;

    bool selected() const;

    void setActivated(bool active);

    void setPreselected(SelectionMode mode);

    void applyPreselection();

protected:
    virtual void selectionCallback();

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    bool m_active;

    bool m_selected;

    SelectionMode m_preSelected;
};

} // End namespace DesignTools.
