/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef REFACTOROVERLAY_H
#define REFACTOROVERLAY_H

#include "texteditor_global.h"

#include <QtGui/QTextCursor>
#include <QtGui/QIcon>

namespace TextEditor {
class BaseTextEditor;

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
    explicit RefactorOverlay(TextEditor::BaseTextEditor *editor);

    bool isEmpty() const { return m_markers.isEmpty(); }
    void paint(QPainter *painter, const QRect &clip);

    void setMarkers(const RefactorMarkers &markers) { m_markers = markers; }
    RefactorMarkers markers() const { return m_markers; }

    void clear() { m_markers.clear(); }

    RefactorMarker markerAt(const QPoint &pos) const;

private:
    void paintMarker(const RefactorMarker& marker, QPainter *painter, const QRect &clip);
    RefactorMarkers m_markers;
    BaseTextEditor *m_editor;
    int m_maxWidth;
    const QIcon m_icon;
};

} // namespace TextEditor

#endif // REFACTOROVERLAY_H
