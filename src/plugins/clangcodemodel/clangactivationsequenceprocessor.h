// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cplusplus/Token.h>

#include <QString>

namespace ClangCodeModel {
namespace Internal {

class ActivationSequenceProcessor
{
public:
    ActivationSequenceProcessor(const QString &activationString,
                                int positionInDocument,
                                bool wantFunctionCall);

    CPlusPlus::Kind completionKind() const;
    int offset() const;
    int operatorStartPosition() const; // e.g. points to '.' for "foo.bar<CURSOR>"

private:
    void extractCharactersBeforePosition(const QString &activationString);
    void process();
    void processDot();
    void processComma();
    void processLeftParen();
    void processLeftBrace();
    void processColonColon();
    void processArrow();
    void processDotStar();
    void processArrowStar();
    void processDoxyGenComment();
    void processAngleStringLiteral();
    void processStringLiteral();
    void processSlash();
    void processPound();

private:
    CPlusPlus::Kind m_completionKind = CPlusPlus::T_EOF_SYMBOL;
    int m_offset = 0;
    int m_positionInDocument;
    QChar m_char1;
    QChar m_char2;
    QChar m_char3;
    bool m_wantFunctionCall;
};

} // namespace Internal
} // namespace ClangCodeModel
