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

#include "sourcefilepathcontainerbase.h"
#include "sourcerangewithtextcontainer.h"

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class SourceRangesContainer : public SourceFilePathContainerBase
{
public:
    SourceRangesContainer() = default;
    SourceRangesContainer(std::unordered_map<uint, FilePath> &&filePathHash,
                          std::vector<SourceRangeWithTextContainer> &&sourceRangeWithTextContainers)
        : SourceFilePathContainerBase(std::move(filePathHash)),
          sourceRangeWithTextContainers_(std::move(sourceRangeWithTextContainers))
    {}

    const FilePath &filePathForSourceRange(const SourceRangeWithTextContainer &sourceRange) const
    {
        auto found = filePathHash.find(sourceRange.fileHash());

        return found->second;
    }

    const std::vector<SourceRangeWithTextContainer> &sourceRangeWithTextContainers() const
    {
        return sourceRangeWithTextContainers_;
    }

    std::vector<SourceRangeWithTextContainer> takeSourceRangeWithTextContainers()
    {
        return std::move(sourceRangeWithTextContainers_);
    }

    bool hasContent() const
    {
        return !sourceRangeWithTextContainers_.empty();
    }

    void insertSourceRange(uint fileId,
                           uint startLine,
                           uint startColumn,
                           uint startOffset,
                           uint endLine,
                           uint endColumn,
                           uint endOffset,
                           Utils::SmallString &&text)
    {
        sourceRangeWithTextContainers_.emplace_back(fileId,
                                                    startLine,
                                                    startColumn,
                                                    startOffset,
                                                    endLine,
                                                    endColumn,
                                                    endOffset,
                                                    std::move(text));
    }

    void reserve(std::size_t size)
    {
        SourceFilePathContainerBase::reserve(size);
        sourceRangeWithTextContainers_.reserve(size);
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceRangesContainer &container)
    {
        out << container.filePathHash;
        out << container.sourceRangeWithTextContainers_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceRangesContainer &container)
    {
        in >> container.filePathHash;
        in >> container.sourceRangeWithTextContainers_;

        return in;
    }

    friend bool operator==(const SourceRangesContainer &first, const SourceRangesContainer &second)
    {
        return first.sourceRangeWithTextContainers_ == second.sourceRangeWithTextContainers_;
    }

    SourceRangesContainer clone() const
    {
        return SourceRangesContainer(Utils::clone(filePathHash), Utils::clone(sourceRangeWithTextContainers_));
    }

    std::vector<SourceRangeWithTextContainer> sourceRangeWithTextContainers_;
};


CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceRangesContainer &container);
void PrintTo(const SourceRangesContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
