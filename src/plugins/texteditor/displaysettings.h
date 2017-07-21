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

#include "texteditor_global.h"

#include "QMetaType"

QT_BEGIN_NAMESPACE
class QSettings;
class QLabel;
QT_END_NAMESPACE

namespace TextEditor {

enum class AnnotationAlignment
{
    NextToContent,
    NextToMargin,
    RightSide
};

class TEXTEDITOR_EXPORT DisplaySettings
{
public:
    DisplaySettings() = default;

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    bool m_displayLineNumbers = true;
    bool m_textWrapping = false;
    bool m_visualizeWhitespace = false;
    bool m_displayFoldingMarkers = true;
    bool m_highlightCurrentLine = false;
    bool m_highlightBlocks = false;
    bool m_animateMatchingParentheses = true;
    bool m_highlightMatchingParentheses = true;
    bool m_markTextChanges = true ;
    bool m_autoFoldFirstComment = true;
    bool m_centerCursorOnScroll = false;
    bool m_openLinksInNextSplit = false;
    bool m_forceOpenLinksInNextSplit = false;
    bool m_displayFileEncoding = false;
    bool m_scrollBarHighlights = true;
    bool m_animateNavigationWithinFile = false;
    int m_animateWithinFileTimeMax = 333; // read only setting
    bool m_displayAnnotations = true;
    AnnotationAlignment m_annotationAlignment = AnnotationAlignment::RightSide;
    int m_minimalAnnotationContent = 15;

    bool equals(const DisplaySettings &ds) const;

    static QLabel *createAnnotationSettingsLink();
};

inline bool operator==(const DisplaySettings &t1, const DisplaySettings &t2) { return t1.equals(t2); }
inline bool operator!=(const DisplaySettings &t1, const DisplaySettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::AnnotationAlignment)
