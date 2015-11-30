/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "activationsequenceprocessor.h"

namespace ClangCodeModel {
namespace Internal {

namespace {

QString truncateActivationStringByPosition(const QString &activationString,
                                           int positionInDocument)
{
    if (positionInDocument == 1)
        return activationString.left(1);

    if (positionInDocument == 2)
        return activationString.left(2);

    return activationString;
}

}

ActivationSequenceProcessor::ActivationSequenceProcessor(const QString &activationString,
                                                         int positionInDocument,
                                                         bool wantFunctionCall)
    : m_positionInDocument(positionInDocument),
      m_wantFunctionCall(wantFunctionCall)
{
    extractCharactersBeforePosition(truncateActivationStringByPosition(activationString,
                                                                       positionInDocument));

    process();
}

CPlusPlus::Kind ActivationSequenceProcessor::completionKind() const
{
    return m_completionKind;
}

int ActivationSequenceProcessor::offset() const
{
    return m_offset;
}

int ActivationSequenceProcessor::operatorStartPosition() const
{
    return m_positionInDocument - m_offset;
}

void ActivationSequenceProcessor::extractCharactersBeforePosition(const QString &activationString)
{
    if (activationString.size() >= 3) {
        m_char1 = activationString[0];
        m_char2 = activationString[1];
        m_char3 = activationString[2];
    } else if (activationString.size() == 2) {
        m_char2 = activationString[0];
        m_char3 = activationString[1];
    } else if (activationString.size() == 1) {
        m_char3 = activationString[0];
    }
}

void ActivationSequenceProcessor::process()
{
    processDot();
    processComma();
    processLeftParen();
    processColonColon();
    processArrow();
    processDotStar();
    processArrowStar();
    processDoxyGenComment();
    processAngleStringLiteral();
    processStringLiteral();
    processSlash();
    processPound();
}

void ActivationSequenceProcessor::processDot()
{
    if (m_char3 == QLatin1Char('.') && m_char2 != QLatin1Char('.')) {
        m_completionKind = CPlusPlus::T_DOT;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processComma()
{
    if (m_char3 == QLatin1Char(',') ) {
        m_completionKind = CPlusPlus::T_COMMA;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processLeftParen()
{
    if (m_char3 == QLatin1Char('(') && m_wantFunctionCall) {
        m_completionKind = CPlusPlus::T_LPAREN;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processColonColon()
{
    if (m_char2 == QLatin1Char(':') && m_char3 == QLatin1Char(':')) {
        m_completionKind = CPlusPlus::T_COLON_COLON;
        m_offset = 2;
    }
}

void ActivationSequenceProcessor::processArrow()
{
    if (m_char2 == QLatin1Char('-') && m_char3 == QLatin1Char('>')) {
        m_completionKind = CPlusPlus::T_ARROW;
        m_offset = 2;
    }
}

void ActivationSequenceProcessor::processDotStar()
{
    if (m_char2 == QLatin1Char('.') && m_char3 == QLatin1Char('*')) {
        m_completionKind = CPlusPlus::T_DOT_STAR;
        m_offset = 2;
    }
}

void ActivationSequenceProcessor::processArrowStar()
{
    if (m_char1 == QLatin1Char('-') && m_char2 == QLatin1Char('>') && m_char3 == QLatin1Char('*')) {
        m_completionKind = CPlusPlus::T_ARROW_STAR;
        m_offset = 3;
    }
}

void ActivationSequenceProcessor::processDoxyGenComment()
{
    if ((m_char2.isNull() || m_char2.isSpace())
            && (m_char3 == QLatin1Char('\\') || m_char3 == QLatin1Char('@'))) {
        m_completionKind = CPlusPlus::T_DOXY_COMMENT;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processAngleStringLiteral()
{
    if (m_char3 == QLatin1Char('<')) {
        m_completionKind = CPlusPlus::T_ANGLE_STRING_LITERAL;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processStringLiteral()
{
    if (m_char3 == QLatin1Char('"')) {
        m_completionKind = CPlusPlus::T_STRING_LITERAL;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processSlash()
{
    if (m_char3 == QLatin1Char('/')) {
        m_completionKind = CPlusPlus::T_SLASH;
        m_offset = 1;
    }
}

void ActivationSequenceProcessor::processPound()
{
    if (m_char3 == QLatin1Char('#')) {
        m_completionKind = CPlusPlus::T_POUND;
        m_offset = 1;
    }
}

} // namespace Internal
} // namespace ClangCodeModel

