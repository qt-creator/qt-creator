// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"

#include <QList>
#include <QObject>

QT_BEGIN_NAMESPACE
class QScrollBar;
QT_END_NAMESPACE

namespace Timeline {

class TimeRuler;
class TrackPainter;
class TrackLabels;
class TimelineZoomControl;

class TRACING_EXPORT TimelineScrollSync : public QObject
{
    Q_OBJECT
public:
    explicit TimelineScrollSync(TimelineZoomControl *zoom, QObject *parent = nullptr);

    void registerRuler(TimeRuler *ruler);
    void registerContent(TrackPainter *painter);
    void registerLabels(TrackLabels *labels);
    void setVerticalScrollBar(QScrollBar *scrollBar);

private:
    TimelineZoomControl *m_zoom;
    QList<TrackLabels *> m_labels;
    QScrollBar *m_scrollBar = nullptr;
};

} // namespace Timeline
