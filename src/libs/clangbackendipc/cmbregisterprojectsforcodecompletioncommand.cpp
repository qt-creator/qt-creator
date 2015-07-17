/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmbregisterprojectsforcodecompletioncommand.h"

#include "container_common.h"

#include <QDataStream>
#include <QDebug>

#include <algorithm>
#include <ostream>

namespace ClangBackEnd {

RegisterProjectPartsForCodeCompletionCommand::RegisterProjectPartsForCodeCompletionCommand(const QVector<ProjectPartContainer> &projectContainers)
    :projectContainers_(projectContainers)
{
}

const QVector<ProjectPartContainer> &RegisterProjectPartsForCodeCompletionCommand::projectContainers() const
{
    return projectContainers_;
}

QDataStream &operator<<(QDataStream &out, const RegisterProjectPartsForCodeCompletionCommand &command)
{
    out << command.projectContainers_;

    return out;
}

QDataStream &operator>>(QDataStream &in, RegisterProjectPartsForCodeCompletionCommand &command)
{
    in >> command.projectContainers_;

    return in;
}

bool operator==(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second)
{
    return first.projectContainers_ == second.projectContainers_;
}

bool operator<(const RegisterProjectPartsForCodeCompletionCommand &first, const RegisterProjectPartsForCodeCompletionCommand &second)
{
    return compareContainer(first.projectContainers_, second.projectContainers_);
}

QDebug operator<<(QDebug debug, const RegisterProjectPartsForCodeCompletionCommand &command)
{
    debug.nospace() << "RegisterProjectPartsForCodeCompletionCommand(";

    for (const ProjectPartContainer &projectContainer : command.projectContainers())
        debug.nospace() << projectContainer<< ", ";

    debug.nospace() << ")";

    return debug;
}

void PrintTo(const RegisterProjectPartsForCodeCompletionCommand &command, ::std::ostream* os)
{
    *os << "RegisterProjectPartsForCodeCompletionCommand(";

    for (const ProjectPartContainer &projectContainer : command.projectContainers())
        PrintTo(projectContainer, os);

    *os << ")";
}

} // namespace ClangBackEnd

