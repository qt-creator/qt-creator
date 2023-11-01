// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include <utils/id.h>
#include <utils/theme/theme.h>

#include <QHash>
#include <QPointer>
#include <QVector>

QT_BEGIN_NAMESPACE
class QAbstractScrollArea;
class QScrollBar;
QT_END_NAMESPACE

namespace Core {

struct CORE_EXPORT Highlight
{
    enum Priority {
        Invalid = -1,
        LowPriority = 0,
        NormalPriority = 1,
        HighPriority = 2,
        HighestPriority = 3
    };

    Highlight(Utils::Id category, int position, Utils::Theme::Color color, Priority priority);
    Highlight() = default;

    Utils::Id category;
    int position = -1;
    Utils::Theme::Color color = Utils::Theme::TextColorNormal;
    Priority priority = Invalid;
};

class HighlightScrollBarOverlay;

class CORE_EXPORT HighlightScrollBarController
{
public:
    HighlightScrollBarController() = default;
    ~HighlightScrollBarController();

    QScrollBar *scrollBar() const;
    QAbstractScrollArea *scrollArea() const;
    void setScrollArea(QAbstractScrollArea *scrollArea);

    double lineHeight() const;
    void setLineHeight(double lineHeight);

    double visibleRange() const;
    void setVisibleRange(double visibleRange);

    double margin() const;
    void setMargin(double margin);

    QHash<Utils::Id, QVector<Highlight>> highlights() const;
    void addHighlight(Highlight highlight);

    void removeHighlights(Utils::Id id);
    void removeAllHighlights();

private:
    QHash<Utils::Id, QVector<Highlight> > m_highlights;
    double m_lineHeight = 0.0;
    double m_visibleRange = 0.0; // in pixels
    double m_margin = 0.0;       // in pixels
    QAbstractScrollArea *m_scrollArea = nullptr;
    QPointer<HighlightScrollBarOverlay> m_overlay;
};

} // namespace Core
