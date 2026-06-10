// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "timelinemodel.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFormLayout;
class QFrame;
class QLabel;
class QToolButton;
QT_END_NAMESPACE

namespace Timeline {

class TRACING_EXPORT RangeDetailsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit RangeDetailsWidget(QWidget *parent = nullptr);

    void setData(const QString &title, const QList<QPair<QString, QString>> &content);
    void clear();

    bool locked() const { return m_locked; }
    void setLocked(bool locked);

signals:
    void lockChanged(bool locked);
    void recenterOnItem();

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

private:
    void rebuildRows(const QList<QPair<QString, QString>> &content);
    void fitHeight();

    QWidget *m_titleBar;
    QLabel *m_titleLabel;
    QToolButton *m_lockBtn;
    QToolButton *m_collapseBtn;
    QFrame *m_contentFrame;
    QWidget *m_formWidget;
    QFormLayout *m_form;

    QPoint m_dragOffset;
    bool m_dragging = false;
    bool m_didDrag = false;
    bool m_locked = false;
    bool m_collapsed = false;
};

} // namespace Timeline
