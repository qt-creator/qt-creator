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

#pragma once

#include <QMap>
#include <QScrollBar>
#include <QSet>

#include <coreplugin/core_global.h>
#include <coreplugin/id.h>
#include <utils/theme/theme.h>

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

    Highlight(Id category, int position, Utils::Theme::Color color, Priority priority);
    Highlight() = default;

    Id category;
    int position = -1;
    Utils::Theme::Color color = Utils::Theme::TextColorNormal;
    Priority priority = Invalid;
};

class HighlightScrollBarOverlay;

class CORE_EXPORT HighlightScrollBar : public QScrollBar
{
    Q_OBJECT

public:
    HighlightScrollBar(Qt::Orientation orientation, QWidget *parent = 0);
    ~HighlightScrollBar() override;

    void setVisibleRange(float visibleRange);
    void setRangeOffset(float offset);

    void addHighlight(Highlight highlight);

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

} // namespace Core
