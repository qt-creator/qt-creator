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

#include "projectpartcontainer.h"

#include <QDataStream>
#include <QVector>

namespace ClangBackEnd {

class RegisterProjectPartsForEditorMessage
{
public:
    RegisterProjectPartsForEditorMessage() = default;
    RegisterProjectPartsForEditorMessage(const QVector<ProjectPartContainer> &projectContainers)
        :m_projectContainers(projectContainers)
    {
    }

    const QVector<ProjectPartContainer> &projectContainers() const
    {
        return m_projectContainers;
    }

    friend QDataStream &operator<<(QDataStream &out, const RegisterProjectPartsForEditorMessage &message)
    {
        out << message.m_projectContainers;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, RegisterProjectPartsForEditorMessage &message)
    {
        in >> message.m_projectContainers;

        return in;
    }

    friend bool operator==(const RegisterProjectPartsForEditorMessage &first, const RegisterProjectPartsForEditorMessage &second)
    {
        return first.m_projectContainers == second.m_projectContainers;
    }

private:
    QVector<ProjectPartContainer> m_projectContainers;
};

CLANGSUPPORT_EXPORT QDebug operator<<(QDebug debug, const RegisterProjectPartsForEditorMessage &message);
std::ostream &operator<<(std::ostream &os, const RegisterProjectPartsForEditorMessage &message);

DECLARE_MESSAGE(RegisterProjectPartsForEditorMessage);
} // namespace ClangBackEnd
