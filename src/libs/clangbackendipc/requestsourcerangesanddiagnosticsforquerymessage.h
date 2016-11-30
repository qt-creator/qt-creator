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

#include "filecontainerv2.h"

namespace ClangBackEnd {

class RequestSourceRangesAndDiagnosticsForQueryMessage
{
public:
    RequestSourceRangesAndDiagnosticsForQueryMessage() = default;
    RequestSourceRangesAndDiagnosticsForQueryMessage(Utils::SmallString &&query,
                                                     std::vector<V2::FileContainer> &&sources,
                                                     std::vector<V2::FileContainer> &&unsavedContent)
        : query_(std::move(query)),
          sources_(std::move(sources)),
          unsavedContent_(std::move(unsavedContent))

    {}

    const std::vector<V2::FileContainer> &sources() const
    {
        return sources_;
    }

    std::vector<V2::FileContainer> takeSources()
    {
        return std::move(sources_);
    }

    const std::vector<V2::FileContainer> &unsavedContent() const
    {
        return unsavedContent_;
    }

    std::vector<V2::FileContainer> takeUnsavedContent()
    {
        return std::move(unsavedContent_);
    }

    const Utils::SmallString &query() const
    {
        return query_;
    }

    Utils::SmallString takeQuery()
    {
        return std::move(query_);
    }

    friend QDataStream &operator<<(QDataStream &out, const RequestSourceRangesAndDiagnosticsForQueryMessage &message)
    {
        out << message.query_;
        out << message.sources_;
        out << message.unsavedContent_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RequestSourceRangesAndDiagnosticsForQueryMessage &message)
    {
        in >> message.query_;
        in >> message.sources_;
        in >> message.unsavedContent_;

        return in;
    }

    friend bool operator==(const RequestSourceRangesAndDiagnosticsForQueryMessage &first,
                           const RequestSourceRangesAndDiagnosticsForQueryMessage &second)
    {
        return first.query_ == second.query_
            && first.sources_ == second.sources_
            && first.unsavedContent_ == second.unsavedContent_;
    }

    RequestSourceRangesAndDiagnosticsForQueryMessage clone() const
    {
        return RequestSourceRangesAndDiagnosticsForQueryMessage(query_.clone(),
                                                                Utils::clone(sources_),
                                                                Utils::clone(unsavedContent_));
    }

private:
    Utils::SmallString query_;
    std::vector<V2::FileContainer> sources_;
    std::vector<V2::FileContainer> unsavedContent_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const RequestSourceRangesAndDiagnosticsForQueryMessage &message);
void PrintTo(const RequestSourceRangesAndDiagnosticsForQueryMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(RequestSourceRangesAndDiagnosticsForQueryMessage)

} // namespace ClangBackEnd
