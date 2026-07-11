// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

#include <QHash>
#include <QList>
#include <QObject>
#include <QPair>
#include <QPointer>
#include <QStringList>

namespace TextEditor {

class TextEditorWidget;

// category of the ghost row layout items in Utils::TextEditorLayout
TEXTEDITOR_EXPORT Utils::Id inlineDiffGhostCategory();
// extra selection kind used for the added/changed line highlights
TEXTEDITOR_EXPORT Utils::Id inlineDiffSelectionKind();

// Displays one side of a diff on top of an editable TextEditorWidget: lines
// removed from a baseline are rendered as read-only "ghost rows" between the
// real lines (inline view) or as empty spacer rows that align the widget with
// a second widget showing the other side (side by side view). Added or
// modified lines get a full width background highlight.
class TEXTEDITOR_EXPORT InlineDiffDecorator : public QObject
{
    Q_OBJECT

public:
    // which side of the diff the decorated widget shows, determines the
    // highlighting colors
    enum class DiffSide { Baseline, Editor };

    // (start, length) ranges in line local character positions
    using CharRanges = QList<QPair<int, int>>;

    class TEXTEDITOR_EXPORT GhostBlock
    {
    public:
        // 1-based editor line the removed lines are shown above;
        // lineCount + 1 shows them below the last line
        int anchorLine = 1;
        QStringList lines;
        QList<CharRanges> charHighlights; // one entry per line, may be shorter than lines
    };

    class TEXTEDITOR_EXPORT ChangedRange
    {
    public:
        int startLine = 1; // 1-based, inclusive
        int endLine = 1;   // 1-based, inclusive
        QHash<int, CharRanges> charHighlights; // per 1-based editor line
    };

    class TEXTEDITOR_EXPORT Spacer
    {
    public:
        int anchorLine = 1; // 1-based line the spacer is shown above, like GhostBlock
        int lineCount = 0;
    };

    explicit InlineDiffDecorator(TextEditorWidget *widget, DiffSide side = DiffSide::Editor);
    ~InlineDiffDecorator() override;

    void apply(const QList<GhostBlock> &ghosts, const QList<ChangedRange> &changes,
               const QList<Spacer> &spacers = {});
    void clear();

private:
    void clearGhostItems();
    void stripChangedLineFormats();

    QPointer<TextEditorWidget> m_widget;
    const DiffSide m_side;
    QList<GhostBlock> m_ghosts;
    QList<ChangedRange> m_changes;
    QList<Spacer> m_spacers;
    int m_lastBlockCount = 0;
};

} // namespace TextEditor
