/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef COMPLETIONSETTINGS_H
#define COMPLETIONSETTINGS_H

#include "texteditor_global.h"

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {

enum CaseSensitivity {
    CaseInsensitive,
    CaseSensitive,
    FirstLetterCaseSensitive
};

/**
 * Settings that describe how the code completion behaves.
 */
struct TEXTEDITOR_EXPORT CompletionSettings
{
    CompletionSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    bool equals(const CompletionSettings &bs) const;

    CaseSensitivity m_caseSensitivity;
    bool m_autoInsertBrackets;
    bool m_partiallyComplete;
    bool m_spaceAfterFunctionName;
};

inline bool operator==(const CompletionSettings &t1, const CompletionSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const CompletionSettings &t1, const CompletionSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // COMPLETIONSETTINGS_H
