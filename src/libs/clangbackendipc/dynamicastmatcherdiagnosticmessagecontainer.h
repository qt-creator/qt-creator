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

#include "dynamicmatcherdiagnostics.h"
#include "sourcerangecontainerv2.h"

#include <utils/smallstringio.h>

namespace ClangBackEnd {

class DynamicASTMatcherDiagnosticMessageContainer
{
public:
    DynamicASTMatcherDiagnosticMessageContainer() = default;
    DynamicASTMatcherDiagnosticMessageContainer(V2::SourceRangeContainer &&sourceRange,
                                                ClangQueryDiagnosticErrorType errorType,
                                                Utils::SmallStringVector &&arguments)
        : sourceRange_(sourceRange),
          errorType_(errorType),
          arguments_(std::move(arguments))
    {
    }

    const V2::SourceRangeContainer &sourceRange() const
    {
        return sourceRange_;
    }

    ClangQueryDiagnosticErrorType errorType() const
    {
        return errorType_;
    }

    Utils::SmallString errorTypeText() const;

    const Utils::SmallStringVector &arguments() const
    {
        return arguments_;
    }

    friend QDataStream &operator<<(QDataStream &out, const DynamicASTMatcherDiagnosticMessageContainer &container)
    {
        out << container.sourceRange_;
        out << quint32(container.errorType_);
        out << container.arguments_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, DynamicASTMatcherDiagnosticMessageContainer &container)
    {
        quint32 errorType;

        in >> container.sourceRange_;
        in >> errorType;
        in >> container.arguments_;

        container.errorType_ = static_cast<ClangQueryDiagnosticErrorType>(errorType);

        return in;
    }

    friend bool operator==(const DynamicASTMatcherDiagnosticMessageContainer &first,
                           const DynamicASTMatcherDiagnosticMessageContainer &second)
    {
        return first.errorType_ == second.errorType_
            && first.sourceRange_ == second.sourceRange_
            && first.arguments_ == second.arguments_;
    }

    DynamicASTMatcherDiagnosticMessageContainer clone() const
    {
        return DynamicASTMatcherDiagnosticMessageContainer(sourceRange_.clone(),
                                                           errorType_,
                                                           arguments_.clone());
    }

private:
    V2::SourceRangeContainer sourceRange_;
    ClangQueryDiagnosticErrorType errorType_;
    Utils::SmallStringVector arguments_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const DynamicASTMatcherDiagnosticMessageContainer &container);
void PrintTo(const DynamicASTMatcherDiagnosticMessageContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
