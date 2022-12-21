// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

#include <QTextCursor>
#include <QIcon>

namespace TextEditor {
class TextEditorWidget;

struct TEXTEDITOR_EXPORT RefactorMarker;
using RefactorMarkers = QList<RefactorMarker>;

struct TEXTEDITOR_EXPORT RefactorMarker {
    inline bool isValid() const { return !cursor.isNull(); }
    QTextCursor cursor;
    QString tooltip;
    QIcon icon;
    mutable QRect rect; // used to cache last drawing positin in document coordinates
    std::function<void(TextEditor::TextEditorWidget *)> callback;
    Utils::Id type;
    QVariant data;

    static RefactorMarkers filterOutType(const RefactorMarkers &markers, const Utils::Id &type);
};


class  TEXTEDITOR_EXPORT RefactorOverlay : public QObject
{
    Q_OBJECT
public:
    explicit RefactorOverlay(TextEditor::TextEditorWidget *editor);

    bool isEmpty() const { return m_markers.isEmpty(); }
    void paint(QPainter *painter, const QRect &clip);

    void setMarkers(const RefactorMarkers &markers) { m_markers = markers; }
    RefactorMarkers markers() const { return m_markers; }

    void clear() { m_markers.clear(); }

    RefactorMarker markerAt(const QPoint &pos) const;

private:
    void paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip);
    RefactorMarkers m_markers;
    TextEditorWidget *m_editor;
    int m_maxWidth;
    const QIcon m_icon;
};

} // namespace TextEditor
