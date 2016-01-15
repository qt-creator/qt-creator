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
    int m_automaticProposalTimeoutInMs;
    bool m_autoInsertBrackets;
    bool m_surroundingAutoBrackets;
    bool m_partiallyComplete;
    bool m_spaceAfterFunctionName;
    bool m_autoSplitStrings;
};

inline bool operator==(const CompletionSettings &t1, const CompletionSettings &t2) { return t1.equals(t2); }
inline bool operator!=(const CompletionSettings &t1, const CompletionSettings &t2) { return !t1.equals(t2); }

} // namespace TextEditor

#endif // COMPLETIONSETTINGS_H
