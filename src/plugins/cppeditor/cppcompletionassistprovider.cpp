// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcompletionassistprovider.h"

#include "cpptoolsreuse.h"

#include <cppeditor/cppeditorconstants.h>

#include <cplusplus/Token.h>

using namespace CPlusPlus;

namespace CppEditor {

// ---------------------------
// CppCompletionAssistProvider
// ---------------------------
CppCompletionAssistProvider::CppCompletionAssistProvider(QObject *parent)
    : TextEditor::CompletionAssistProvider(parent)
{}

int CppCompletionAssistProvider::activationCharSequenceLength() const
{
    return 3;
}

bool CppCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    const QChar &ch  = sequence.at(2);
    const QChar &ch2 = sequence.at(1);
    const QChar &ch3 = sequence.at(0);
    return activationSequenceChar(ch, ch2, ch3, nullptr, true, false);
}

bool CppCompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return isValidIdentifierChar(c);
}

int CppCompletionAssistProvider::activationSequenceChar(const QChar &ch,
                                                        const QChar &ch2,
                                                        const QChar &ch3,
                                                        unsigned *kind,
                                                        bool wantFunctionCall,
                                                        bool wantQt5SignalSlots)
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
    case '&':
        if (wantQt5SignalSlots) {
            completionKind = T_AMPER;
            referencePosition = 1;
        }
        break;
    }

    if (kind)
        *kind = completionKind;

    return referencePosition;
}

} // namespace CppEditor
