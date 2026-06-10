// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QWidget>

namespace Timeline {

class RangeDetailsWidget;
class TimelineModelAggregator;
class TimelineZoomControl;

class TRACING_EXPORT TimelineWidget : public QWidget
{
    Q_OBJECT
public:
    explicit TimelineWidget(TimelineModelAggregator *aggregator,
                            TimelineZoomControl *zoomControl,
                            QWidget *parent = nullptr);
    ~TimelineWidget() override;

    // The range details panel, meant to be added to a perspective as a dockable
    // view. Ownership transfers to the perspective via Perspective::addWindow().
    RangeDetailsWidget *rangeDetailsWidget() const;

    bool hasValidSelection() const;
    qint64 selectionStart() const;
    qint64 selectionEnd() const;

    QString currentFile() const;
    int currentLine() const;
    int currentColumn() const;
    int currentTypeId() const;

    void clear();
    void selectByTypeId(int typeId);
    void selectByIndices(int modelIndex, int eventIndex);

signals:
    void gotoSourceLocation(const QString &file, int line, int column);
    void typeSelected(int typeId);

private:
    class TimelineWidgetPrivate;
    TimelineWidgetPrivate *d;
};

} // namespace Timeline
