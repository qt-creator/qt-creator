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
#include "projectpartcontainer.h"

namespace ClangBackEnd {

class UpdateProjectPartsMessage
{
public:
    UpdateProjectPartsMessage() = default;
    UpdateProjectPartsMessage(ProjectPartContainers &&projectsParts,
                              Utils::SmallStringVector &&toolChainArguments)
        : projectsParts(std::move(projectsParts))
        , toolChainArguments(toolChainArguments)
    {}

    ProjectPartContainers takeProjectsParts()
    {
        return std::move(projectsParts);
    }

    friend QDataStream &operator<<(QDataStream &out, const UpdateProjectPartsMessage &message)
    {
        out << message.projectsParts;
        out << message.toolChainArguments;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UpdateProjectPartsMessage &message)
    {
        in >> message.projectsParts;
        in >> message.toolChainArguments;

        return in;
    }

    friend bool operator==(const UpdateProjectPartsMessage &first,
                           const UpdateProjectPartsMessage &second)
    {
        return first.projectsParts == second.projectsParts
               && first.toolChainArguments == second.toolChainArguments;
    }

    UpdateProjectPartsMessage clone() const
    {
        return *this;
    }

public:
    ProjectPartContainers projectsParts;
    Utils::SmallStringVector toolChainArguments;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const UpdateProjectPartsMessage &message);

DECLARE_MESSAGE(UpdateProjectPartsMessage)

} // namespace ClangBackEnd
