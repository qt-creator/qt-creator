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
#include "sourcelocationcontainerv2.h"

#include <utils/smallstringvector.h>

namespace ClangBackEnd {

class SourceLocationsContainer : public SourceFilePathContainerBase
{
public:
    SourceLocationsContainer() = default;
    SourceLocationsContainer(std::unordered_map<uint, FilePath> &&filePathHash,
                             std::vector<V2::SourceLocationContainer> &&sourceLocationContainers)
        : SourceFilePathContainerBase(std::move(filePathHash)),
          sourceLocationContainers_(std::move(sourceLocationContainers))
    {}

    const FilePath &filePathForSourceLocation(const V2::SourceLocationContainer &sourceLocation) const
    {
        auto found = filePathHash.find(sourceLocation.fileHash());

        return found->second;
    }

    const std::vector<V2::SourceLocationContainer> &sourceLocationContainers() const
    {
        return sourceLocationContainers_;
    }

    bool hasContent() const
    {
        return !sourceLocationContainers_.empty();
    }

    void insertSourceLocation(uint fileId, uint line, uint column, uint offset)
    {
        sourceLocationContainers_.emplace_back(fileId, line, column, offset);
    }

    void reserve(std::size_t size)
    {
        SourceFilePathContainerBase::reserve(size);
        sourceLocationContainers_.reserve(size);
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationsContainer &container)
    {
        out << container.filePathHash;
        out << container.sourceLocationContainers_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationsContainer &container)
    {
        in >> container.filePathHash;
        in >> container.sourceLocationContainers_;

        return in;
    }

    friend bool operator==(const SourceLocationsContainer &first, const SourceLocationsContainer &second)
    {
        return first.sourceLocationContainers_ == second.sourceLocationContainers_;
    }

    SourceLocationsContainer clone() const
    {
        return SourceLocationsContainer(Utils::clone(filePathHash), Utils::clone(sourceLocationContainers_));
    }

    std::vector<V2::SourceLocationContainer> sourceLocationContainers_;
};


CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceLocationsContainer &container);
void PrintTo(const SourceLocationsContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd

