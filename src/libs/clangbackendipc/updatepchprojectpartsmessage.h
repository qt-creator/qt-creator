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
#include "projectpartcontainerv2.h"

namespace ClangBackEnd {

class UpdatePchProjectPartsMessage
{
public:
    UpdatePchProjectPartsMessage() = default;
    UpdatePchProjectPartsMessage(V2::ProjectPartContainers &&projectsParts,
                                 V2::FileContainers &&generatedFiles)
        : projectsParts_(std::move(projectsParts)),
          generatedFiles_(std::move(generatedFiles))
    {}

    const V2::ProjectPartContainers &projectsParts() const
    {
        return projectsParts_;
    }

    V2::ProjectPartContainers takeProjectsParts()
    {
        return std::move(projectsParts_);
    }

    const V2::FileContainers &generatedFiles() const
    {
        return generatedFiles_;
    }

    V2::FileContainers takeGeneratedFiles()
    {
        return std::move(generatedFiles_);
    }

    friend QDataStream &operator<<(QDataStream &out, const UpdatePchProjectPartsMessage &message)
    {
        out << message.projectsParts_;
        out << message.generatedFiles_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UpdatePchProjectPartsMessage &message)
    {
        in >> message.projectsParts_;
        in >> message.generatedFiles_;

        return in;
    }

    friend bool operator==(const UpdatePchProjectPartsMessage &first,
                           const UpdatePchProjectPartsMessage &second)
    {
        return first.projectsParts_ == second.projectsParts_
            && first.generatedFiles_ == second.generatedFiles_;
    }

    UpdatePchProjectPartsMessage clone() const
    {
        return UpdatePchProjectPartsMessage(Utils::clone(projectsParts_),
                                            Utils::clone(generatedFiles_));
    }

private:
    V2::ProjectPartContainers projectsParts_;
    V2::FileContainers generatedFiles_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const UpdatePchProjectPartsMessage &message);
std::ostream &operator<<(std::ostream &out, const UpdatePchProjectPartsMessage &message);

DECLARE_MESSAGE(UpdatePchProjectPartsMessage)

} // namespace ClangBackEnd
