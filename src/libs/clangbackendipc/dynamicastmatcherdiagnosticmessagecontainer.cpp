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

#include "dynamicastmatcherdiagnosticmessagecontainer.h"

#define RETURN_CASE(name) \
    case ClangQueryDiagnosticErrorType::name: return #name;

namespace ClangBackEnd {

QDebug operator<<(QDebug debug, const DynamicASTMatcherDiagnosticMessageContainer &container)
{
    debug.nospace() << "DynamicASTMatcherDiagnosticMessageContainer("
                    << container.errorTypeText() << ", "
                    << container.sourceRange() << ", "
                    << container.arguments()
                    << ")";

    return debug;
}

std::ostream &operator<<(std::ostream &os, const DynamicASTMatcherDiagnosticMessageContainer &container)
{
    os << "("
       << container.errorTypeText() << ", "
       << container.sourceRange() << ", "
       << container.arguments()
       << ")";

    return os;
}

Utils::SmallString DynamicASTMatcherDiagnosticMessageContainer::errorTypeText() const
{
    switch (m_errorType) {
       RETURN_CASE(None)
       RETURN_CASE(RegistryMatcherNotFound)
       RETURN_CASE(RegistryWrongArgCount)
       RETURN_CASE(RegistryWrongArgType)
       RETURN_CASE(RegistryNotBindable)
       RETURN_CASE(RegistryAmbiguousOverload)
       RETURN_CASE(RegistryValueNotFound)
       RETURN_CASE(ParserStringError)
       RETURN_CASE(ParserNoOpenParen)
       RETURN_CASE(ParserNoCloseParen)
       RETURN_CASE(ParserNoComma)
       RETURN_CASE(ParserNoCode)
       RETURN_CASE(ParserNotAMatcher)
       RETURN_CASE(ParserInvalidToken)
       RETURN_CASE(ParserMalformedBindExpr)
       RETURN_CASE(ParserTrailingCode)
       RETURN_CASE(ParserUnsignedError)
       RETURN_CASE(ParserOverloadedType)
    }

    Q_UNREACHABLE();
}

} // namespace ClangBackEnd
