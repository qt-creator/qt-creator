/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
    bool m_markTextChanges;
    bool m_autoFoldFirstComment;
    bool m_centerCursorOnScroll;

    bool equals(const DisplaySettings &ds) const;
};

inline bool operator==(const DisplaySettings &t1, const DisplaySettings &t2) { return t1.equals(t2); }
inline bool operator!=(const DisplaySettings &t1, const DisplaySettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // DISPLAYSETTINGS_H
