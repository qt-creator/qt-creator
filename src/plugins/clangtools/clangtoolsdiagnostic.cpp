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

#include "clangtoolsdiagnostic.h"

namespace ClangTools {
namespace Internal {

bool ExplainingStep::isValid() const
{
    return location.isValid() && !ranges.isEmpty() && !message.isEmpty();
}

bool operator==(const ExplainingStep &lhs, const ExplainingStep &rhs)
{
    return lhs.message == rhs.message
        && lhs.location == rhs.location
        && lhs.ranges == rhs.ranges
        && lhs.isFixIt == rhs.isFixIt
        ;
}

bool Diagnostic::isValid() const
{
    return !description.isEmpty();
}

quint32 qHash(const Diagnostic &diagnostic)
{
    return qHash(diagnostic.description)
         ^ qHash(diagnostic.location.filePath)
         ^ diagnostic.location.line
         ^ diagnostic.location.column;
}

bool operator==(const Diagnostic &lhs, const Diagnostic &rhs)
{
    return lhs.description == rhs.description
        && lhs.category == rhs.category
        && lhs.type == rhs.type
        && lhs.location == rhs.location
        && lhs.explainingSteps == rhs.explainingSteps
        && lhs.hasFixits == rhs.hasFixits
        ;
}

} // namespace Internal
} // namespace ClangTools
