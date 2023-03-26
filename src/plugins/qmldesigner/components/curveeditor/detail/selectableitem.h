// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QGraphicsObject>

namespace QmlDesigner {

class CurveEditorItem : public QGraphicsObject
{
public:
    CurveEditorItem(QGraphicsItem *parent);

    virtual void lockedCallback();
    virtual void pinnedCallback();
    virtual void underMouseCallback();

    bool locked() const;
    bool pinned() const;
    bool isUnderMouse() const;

    void setLocked(bool locked);
    void setPinned(bool pinned);
    void setIsUnderMouse(bool under);

private:
    bool m_locked;
    bool m_pinned;
    bool m_underMouse;
};

enum ItemType {
    ItemTypeKeyframe = QGraphicsItem::UserType + 1,
    ItemTypeHandle = QGraphicsItem::UserType + 2,
    ItemTypeCurve = QGraphicsItem::UserType + 3
};


class SelectableItem : public CurveEditorItem
{
    Q_OBJECT

public:
    enum class SelectionMode : unsigned int { Undefined, Clear, New, Add, Remove, Toggle };

    SelectableItem(QGraphicsItem *parent = nullptr);

    ~SelectableItem() override;

    void lockedCallback() override;

    bool activated() const;

    bool selected() const;

    void setActivated(bool active);

    void setSelected(bool selected);

    void setPreselected(SelectionMode mode);

    void applyPreselection();

protected:
    virtual void activationCallback();

    virtual void selectionCallback();

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    bool m_active;

    bool m_selected;

    SelectionMode m_preSelected;
};

} // End namespace QmlDesigner.
