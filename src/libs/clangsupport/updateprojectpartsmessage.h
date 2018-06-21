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

class UpdateProjectPartsMessage
{
public:
    UpdateProjectPartsMessage() = default;
    UpdateProjectPartsMessage(V2::ProjectPartContainers &&projectsParts,
                              V2::FileContainers &&generatedFiles)
        : projectsParts(std::move(projectsParts)),
          generatedFiles(std::move(generatedFiles))
    {}

    V2::ProjectPartContainers takeProjectsParts()
    {
        return std::move(projectsParts);
    }

    V2::FileContainers takeGeneratedFiles()
    {
        return std::move(generatedFiles);
    }

    friend QDataStream &operator<<(QDataStream &out, const UpdateProjectPartsMessage &message)
    {
        out << message.projectsParts;
        out << message.generatedFiles;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UpdateProjectPartsMessage &message)
    {
        in >> message.projectsParts;
        in >> message.generatedFiles;

        return in;
    }

    friend bool operator==(const UpdateProjectPartsMessage &first,
                           const UpdateProjectPartsMessage &second)
    {
        return first.projectsParts == second.projectsParts
            && first.generatedFiles == second.generatedFiles;
    }

    UpdateProjectPartsMessage clone() const
    {
        return UpdateProjectPartsMessage(Utils::clone(projectsParts),
                                         Utils::clone(generatedFiles));
    }

public:
    V2::ProjectPartContainers projectsParts;
    V2::FileContainers generatedFiles;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const UpdateProjectPartsMessage &message);

DECLARE_MESSAGE(UpdateProjectPartsMessage)

} // namespace ClangBackEnd
