// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QList>
#include <QObject>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace TextEditor {

class TextEditorWidget;

// A merge conflict block delimited by the "<<<<<<<", "=======" and ">>>>>>>"
// markers. Line numbers are 1-based; baseLine is only set for the diff3
// conflict style, which adds a "|||||||" base section.
class TEXTEDITOR_EXPORT MergeConflict
{
public:
    int startLine = -1;     // <<<<<<<
    int baseLine = -1;      // |||||||
    int separatorLine = -1; // =======
    int endLine = -1;       // >>>>>>>

    bool operator==(const MergeConflict &o) const
    {
        return startLine == o.startLine && baseLine == o.baseLine
               && separatorLine == o.separatorLine && endLine == o.endLine;
    }
};

// Which side(s) of a conflict to keep when resolving it.
enum class MergeConflictChoice { Current, Incoming, Both };

// Scans the document for merge conflict blocks, in document order.
TEXTEDITOR_EXPORT QList<MergeConflict> findMergeConflicts(const QTextDocument *doc);

// Replaces the conflict block with the chosen side(s), as a single undo step.
TEXTEDITOR_EXPORT void resolveMergeConflict(QTextDocument *doc, const MergeConflict &conflict,
                                            MergeConflictChoice choice);

// objectName of the row of resolution links, for tests to find them by
constexpr char MERGE_CONFLICT_CHOICES_OBJECT_NAME[] = "TextEditor.MergeConflict.Choices";

// Attaches to a text editor and, whenever its document contains merge conflict
// markers, shows a row of "Choose Current Change | Choose Incoming Change |
// Choose Both" links on a reserved line directly above each conflict's
// "<<<<<<<" marker, so conflicts can be resolved without a diff view. A
// read-only view gets the side highlighting but no links. Owned by the widget
// it decorates.
class MergeConflictController : public QObject
{
    Q_OBJECT

public:
    explicit MergeConflictController(TextEditorWidget *widget);
    ~MergeConflictController() override;

    // Takes the reserved lines, the links and the highlighting back out of the
    // widget. Call this before deleting a controller of a widget that lives
    // on; a controller that dies with its widget has nothing to give back.
    void detach();

private:
    bool eventFilter(QObject *watched, QEvent *event) override;

    class MergeConflictControllerPrivate *d;
};

} // namespace TextEditor
