/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#ifndef HIGHLIGHTSCROLLBAR_H
#define HIGHLIGHTSCROLLBAR_H

#include <QMap>
#include <QScrollBar>
#include <QSet>

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <utils/theme/theme.h>

namespace Core {

class HighlightScrollBarOverlay;

class CORE_EXPORT HighlightScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    HighlightScrollBar(Qt::Orientation orientation, QWidget *parent = 0);
    ~HighlightScrollBar() override;

    void setVisibleRange(float visibleRange);
    void setRangeOffset(float offset);
    void setColor(Id category, Utils::Theme::Color color);

    enum Priority
    {
        LowPriority = 0,
        NormalPriority = 1,
        HighPriority = 2,
        HighestPriority = 3
    };

    void setPriority(Id category, Priority prio);
    void addHighlight(Id category, int highlight);
    void addHighlights(Id category, QSet<int> highlights);

    void removeHighlights(Id id);
    void removeAllHighlights();

    bool eventFilter(QObject *, QEvent *event) override;

protected:
    void moveEvent(QMoveEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void changeEvent(QEvent *even) override;

private:
    QRect overlayRect();
    void overlayDestroyed();

    QWidget *m_widget;
    HighlightScrollBarOverlay *m_overlay;
    friend class HighlightScrollBarOverlay;
};

class HighlightScrollBarOverlay : public QWidget
{
    Q_OBJECT
public:
    HighlightScrollBarOverlay(HighlightScrollBar *scrollBar)
        : QWidget(scrollBar)
        , m_visibleRange(0.0)
        , m_offset(0.0)
        , m_cacheUpdateScheduled(false)
        , m_scrollBar(scrollBar)
    {}

    void scheduleUpdate();
    void updateCache();
    void adjustPosition();

    float m_visibleRange;
    float m_offset;
    QHash<Id, QSet<int> > m_highlights;
    QHash<Id, Utils::Theme::Color> m_colors;
    QHash<Id, HighlightScrollBar::Priority> m_priorities;

    bool m_cacheUpdateScheduled;
    QMap<int, Id> m_cache;

protected:
    void paintEvent(QPaintEvent *paintEvent) override;

private:
    HighlightScrollBar *m_scrollBar;
};

} // namespace Core

#endif // HIGHLIGHTSCROLLBAR_H
