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

#include "cmbregisterprojectsforeditormessage.h"

#include <QDataStream>
#include <QDebug>

#include <algorithm>
#include <ostream>

namespace ClangBackEnd {

RegisterProjectPartsForEditorMessage::RegisterProjectPartsForEditorMessage(const QVector<ProjectPartContainer> &projectContainers)
    :projectContainers_(projectContainers)
{
}

const QVector<ProjectPartContainer> &RegisterProjectPartsForEditorMessage::projectContainers() const
{
    return projectContainers_;
}

QDataStream &operator<<(QDataStream &out, const RegisterProjectPartsForEditorMessage &message)
{
    out << message.projectContainers_;

    return out;
}

QDataStream &operator>>(QDataStream &in, RegisterProjectPartsForEditorMessage &message)
{
    in >> message.projectContainers_;

    return in;
}

bool operator==(const RegisterProjectPartsForEditorMessage &first, const RegisterProjectPartsForEditorMessage &second)
{
    return first.projectContainers_ == second.projectContainers_;
}

QDebug operator<<(QDebug debug, const RegisterProjectPartsForEditorMessage &message)
{
    debug.nospace() << "RegisterProjectPartsForEditorMessage(";

    for (const ProjectPartContainer &projectContainer : message.projectContainers())
        debug.nospace() << projectContainer<< ", ";

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const RegisterProjectPartsForEditorMessage &message, ::std::ostream* os)
{
    *os << "RegisterProjectPartsForEditorMessage(";

    for (const ProjectPartContainer &projectContainer : message.projectContainers())
        PrintTo(projectContainer, os);

    *os << ")";
}

} // namespace ClangBackEnd

