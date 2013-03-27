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

#include "cppcompletionassistprovider.h"

#include <cppeditor/cppeditorconstants.h>

#include <cplusplus/Token.h>

using namespace CPlusPlus;
using namespace CppTools;

// ---------------------------
// CppCompletionAssistProvider
// ---------------------------
bool CppCompletionAssistProvider::supportsEditor(const Core::Id &editorId) const
{
    return editorId == CppEditor::Constants::CPPEDITOR_ID;
}

int CppCompletionAssistProvider::activationCharSequenceLength() const
{
    return 3;
}

bool CppCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    const QChar &ch  = sequence.at(2);
    const QChar &ch2 = sequence.at(1);
    const QChar &ch3 = sequence.at(0);
    if (activationSequenceChar(ch, ch2, ch3, 0, true) != 0)
        return true;
    return false;
}

int CppCompletionAssistProvider::activationSequenceChar(const QChar &ch,
                                                        const QChar &ch2,
                                                        const QChar &ch3,
                                                        unsigned *kind,
                                                        bool wantFunctionCall)
{
    int referencePosition = 0;
    int completionKind = T_EOF_SYMBOL;
    switch (ch.toLatin1()) {
    case '.':
        if (ch2 != QLatin1Char('.')) {
            completionKind = T_DOT;
            referencePosition = 1;
        }
        break;
    case ',':
        completionKind = T_COMMA;
        referencePosition = 1;
        break;
    case '(':
        if (wantFunctionCall) {
            completionKind = T_LPAREN;
            referencePosition = 1;
        }
        break;
    case ':':
        if (ch3 != QLatin1Char(':') && ch2 == QLatin1Char(':')) {
            completionKind = T_COLON_COLON;
            referencePosition = 2;
        }
        break;
    case '>':
        if (ch2 == QLatin1Char('-')) {
            completionKind = T_ARROW;
            referencePosition = 2;
        }
        break;
    case '*':
        if (ch2 == QLatin1Char('.')) {
            completionKind = T_DOT_STAR;
            referencePosition = 2;
        } else if (ch3 == QLatin1Char('-') && ch2 == QLatin1Char('>')) {
            completionKind = T_ARROW_STAR;
            referencePosition = 3;
        }
        break;
    case '\\':
    case '@':
        if (ch2.isNull() || ch2.isSpace()) {
            completionKind = T_DOXY_COMMENT;
            referencePosition = 1;
        }
        break;
    case '<':
        completionKind = T_ANGLE_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '"':
        completionKind = T_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '/':
        completionKind = T_SLASH;
        referencePosition = 1;
        break;
    case '#':
        completionKind = T_POUND;
        referencePosition = 1;
        break;
    }

    if (kind)
        *kind = completionKind;

    return referencePosition;
}
