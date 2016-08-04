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

#include "clangbackendipc_global.h"

#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class ProjectPartContainer
{
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(const Utf8String &projectPartId,
                         const Utf8StringVector &arguments = Utf8StringVector())
        : projectPartId_(projectPartId),
          arguments_(arguments)
    {
    }

    const Utf8String &projectPartId() const
    {
        return projectPartId_;
    }

    const Utf8StringVector &arguments() const
    {
        return arguments_;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.projectPartId_;
        out << container.arguments_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.projectPartId_;
        in >> container.arguments_;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.projectPartId_ == second.projectPartId_;
    }

private:
    Utf8String projectPartId_;
    Utf8StringVector arguments_;
};

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
void PrintTo(const ProjectPartContainer &container, ::std::ostream* os);

} // namespace ClangBackEnd
