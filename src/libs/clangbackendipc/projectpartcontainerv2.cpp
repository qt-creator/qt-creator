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

#include "projectpartcontainerv2.h"

namespace ClangBackEnd {
namespace V2 {

QDebug operator<<(QDebug debug, const ProjectPartContainer &container)
{
    debug.nospace() << "ProjectPartContainer("
                    << container.projectPartId() << ","
                    << container.arguments() << ", "
                    << container.headerPaths() << ", "
                    << container.sourcePaths()
                    << ")";

    return debug;
}

std::ostream &operator<<(std::ostream &out, const ProjectPartContainer &container)
{
    out << "("
        << container.projectPartId() << ", "
        << container.arguments() << ", "
        << container.headerPaths() << ", "
        << container.sourcePaths()<< ")";

    return out;
}

} // namespace V2
} // namespace ClangBackEnd
