/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QHash>
#include <QPointer>
#include <QVector>

#include <coreplugin/core_global.h>

#include <utils/id.h>
#include <utils/theme/theme.h>

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
