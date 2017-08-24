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

#include "clangsupport_global.h"

#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class ProjectPartContainer
{
public:
    ProjectPartContainer() = default;
    ProjectPartContainer(const Utf8String &projectPartId,
                         const Utf8StringVector &arguments = Utf8StringVector())
        : m_projectPartId(projectPartId),
          m_arguments(arguments)
    {
    }

    const Utf8String &projectPartId() const
    {
        return m_projectPartId;
    }

    const Utf8StringVector &arguments() const
    {
        return m_arguments;
    }

    friend QDataStream &operator<<(QDataStream &out, const ProjectPartContainer &container)
    {
        out << container.m_projectPartId;
        out << container.m_arguments;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ProjectPartContainer &container)
    {
        in >> container.m_projectPartId;
        in >> container.m_arguments;

        return in;
    }

    friend bool operator==(const ProjectPartContainer &first, const ProjectPartContainer &second)
    {
        return first.m_projectPartId == second.m_projectPartId;
    }

private:
    Utf8String m_projectPartId;
    Utf8StringVector m_arguments;
};

QDebug operator<<(QDebug debug, const ProjectPartContainer &container);
std::ostream &operator<<(std::ostream &os, const ProjectPartContainer &container);

} // namespace ClangBackEnd
