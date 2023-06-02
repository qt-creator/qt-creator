// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QWidget>

#include <optional>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Core::Internal {

class ProgressView : public QWidget
{
    Q_OBJECT

public:
    ProgressView(QWidget *parent = nullptr);
    ~ProgressView() override;

    void addProgressWidget(QWidget *widget);
    void removeProgressWidget(QWidget *widget);

    bool isHovered() const;

    void setReferenceWidget(QWidget *widget);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *ev) override;
    void mouseMoveEvent(QMouseEvent *ev) override;
    void mouseReleaseEvent(QMouseEvent *ev) override;

signals:
    void hoveredChanged(bool hovered);

private:
    void reposition();
    QPoint topRightReferenceInParent() const;

    QVBoxLayout *m_layout;
    QWidget *m_referenceWidget = nullptr;
    QWidget *m_pinButton = nullptr;

    // dragging
    std::optional<QPointF> m_clickPosition;
    QPointF m_clickPositionInWidget;
    bool m_isDragging = false;

    // relative to referenceWidget's topRight in parentWidget()
    // can be changed by the user by dragging
    QPoint m_anchorBottomRight;

    bool m_hovered = false;
};

} // Core::Internal
