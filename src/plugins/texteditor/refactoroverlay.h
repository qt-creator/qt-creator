/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef REFACTOROVERLAY_H
#define REFACTOROVERLAY_H

#include "texteditor_global.h"

#include <QTextCursor>
#include <QIcon>

namespace TextEditor {
class TextEditorWidget;

struct  TEXTEDITOR_EXPORT RefactorMarker {
    inline bool isValid() const { return !cursor.isNull(); }
    QTextCursor cursor;
    QString tooltip;
    QIcon icon;
    mutable QRect rect; // used to cache last drawing positin in document coordinates
    QVariant data;
};

typedef QList<RefactorMarker> RefactorMarkers;

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

#endif // REFACTOROVERLAY_H
