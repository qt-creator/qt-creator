// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QColor>
#include <QList>
#include <QPoint>
#include <QString>
#include <QWidget>

namespace Timeline {

struct TrackInfo
{
    QString name;
    QString tooltip;      // hover tooltip (falls back to name if empty)
    QColor color;         // category accent strip color
    bool expanded = false;
    bool empty = false;    // model has no items (expand button disabled)
    QStringList rowLabels; // sub-row names when expanded (empty when collapsed)
    QList<int> rowLabelIds; // selection IDs parallel to rowLabels
    QList<int> rowHeights; // rowHeights[0] = title row; [1..] = sub-rows
    QList<int> noteEventIds; // timeline event indices for notes on this model
    QStringList noteTexts;   // note texts parallel to noteEventIds
};

class TRACING_EXPORT TrackLabels : public QWidget
{
    Q_OBJECT
public:
    explicit TrackLabels(QWidget *parent = nullptr);

    void setTracks(const QList<TrackInfo> &tracks);
    void setScrollOffset(int y);

    int scrollOffset() const { return m_scrollOffset; }

    QSize sizeHint() const override;

signals:
    void expandToggled(int trackIndex);
    void moveCategories(int sourceIndex, int targetIndex);
    void rowLabelClicked(int trackIndex, int rowIndex, bool reverse);
    void rowHeightChangeRequested(int trackIndex, int rowIndex, int newHeight);
    void noteClicked(int trackIndex, int eventId);

protected:
    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QList<TrackInfo> m_tracks;
    int m_scrollOffset = 0;
    int m_totalHeight = 0;
    int m_dragSource = -1;
    int m_dragInsertSlot = -1;
    QPoint m_dragPressPos;
    bool m_dragging = false;
    int m_resizeTrack = -1;
    int m_resizeRow = -1;
    int m_resizeStartY = 0;
    int m_resizeOrigHeight = 0;
    QList<int> m_noteCurrentIdx;

    void updateTotalHeight();
};

} // namespace Timeline
