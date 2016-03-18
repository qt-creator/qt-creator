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
