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

enum CompletionTrigger {
    ManualCompletion,     // Display proposal only when explicitly invoked by the user.
    TriggeredCompletion,  // When triggered by the user or upon contextual activation characters.
    AutomaticCompletion   // The above plus an automatic trigger when the editor is "idle".
};

/**
 * Settings that describe how the code completion behaves.
 */
class TEXTEDITOR_EXPORT CompletionSettings
{
public:
    CompletionSettings();

    void toSettings(const QString &category, QSettings *s) const;
    void fromSettings(const QString &category, const QSettings *s);

    bool equals(const CompletionSettings &bs) const;

    CaseSensitivity m_caseSensitivity;
    CompletionTrigger m_completionTrigger;
    bool m_autoInsertBrackets;
    bool m_surroundingAutoBrackets;
    bool m_partiallyComplete;
    bool m_spaceAfterFunctionName;
};

inline bool operator==(const CompletionSettings &t1, const CompletionSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const CompletionSettings &t1, const CompletionSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // COMPLETIONSETTINGS_H
