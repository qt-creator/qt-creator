// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

#include <QTextCursor>
#include <QIcon>

namespace TextEditor {
class TextEditorWidget;

struct TEXTEDITOR_EXPORT RefactorMarker
{
    bool isValid() const { return !cursor.isNull(); }

    QTextCursor cursor;
    QString tooltip;
    QIcon icon;
    mutable QRect rect; // used to cache last drawing positin in document coordinates
    std::function<void(TextEditor::TextEditorWidget *)> callback;
    Utils::Id type;
    QVariant data;
};

using RefactorMarkers = QList<RefactorMarker>;

class TEXTEDITOR_EXPORT RefactorOverlay final
{
public:
    explicit RefactorOverlay(TextEditor::TextEditorWidget *editor);

    bool isEmpty() const { return m_markers.isEmpty(); }
    void paint(QPainter *painter, const QRect &clip) const;

    void setMarkers(const RefactorMarkers &markers) { m_markers = markers; }
    RefactorMarkers markers() const { return m_markers; }

    void clear() { m_markers.clear(); }

    RefactorMarker markerAt(const QPoint &pos) const;

private:
    void paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip) const;

    RefactorMarkers m_markers;
    TextEditorWidget *m_editor;
    mutable int m_maxWidth = 0;
    const QIcon m_icon;
};

} // namespace TextEditor
