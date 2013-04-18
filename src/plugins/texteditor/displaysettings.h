/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DISPLAYSETTINGS_H
#define DISPLAYSETTINGS_H

#include "texteditor_global.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

class TEXTEDITOR_EXPORT DisplaySettings
{
public:
    DisplaySettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    bool m_displayLineNumbers;
    bool m_textWrapping;
    bool m_showWrapColumn;
    int m_wrapColumn;
    bool m_visualizeWhitespace;
    bool m_displayFoldingMarkers;
    bool m_highlightCurrentLine;
    bool m_highlightBlocks;
    bool m_animateMatchingParentheses;
    bool m_highlightMatchingParentheses;
    bool m_markTextChanges;
    bool m_autoFoldFirstComment;
    bool m_centerCursorOnScroll;
    bool m_openLinksInNextSplit;
    bool m_forceOpenLinksInNextSplit;
    bool m_displayFileEncoding;

    bool equals(const DisplaySettings &ds) const;
};

inline bool operator==(const DisplaySettings &t1, const DisplaySettings &t2) { return t1.equals(t2); }
inline bool operator!=(const DisplaySettings &t1, const DisplaySettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // DISPLAYSETTINGS_H
