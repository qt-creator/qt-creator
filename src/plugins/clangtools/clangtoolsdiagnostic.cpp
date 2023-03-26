// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangtoolsdiagnostic.h"

#include <utils/utilsicons.h>

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

QIcon Diagnostic::icon() const
{
    if (type == "warning")
        return Utils::Icons::CODEMODEL_WARNING.icon();
    if (type == "error" || type == "fatal")
        return Utils::Icons::CODEMODEL_ERROR.icon();
    if (type == "note")
        return Utils::Icons::INFO.icon();
    if (type == "fix-it")
        return Utils::Icons::CODEMODEL_FIXIT.icon();
    return {};
}

size_t qHash(const Diagnostic &diagnostic)
{
    return qHash(diagnostic.name)
         ^ qHash(diagnostic.description)
         ^ qHash(diagnostic.location.filePath)
         ^ diagnostic.location.line
         ^ diagnostic.location.column;
}

bool operator==(const Diagnostic &lhs, const Diagnostic &rhs)
{
    return lhs.name == rhs.name
        && lhs.description == rhs.description
        && lhs.category == rhs.category
        && lhs.type == rhs.type
        && lhs.location == rhs.location
        && lhs.explainingSteps == rhs.explainingSteps
        && lhs.hasFixits == rhs.hasFixits
        ;
}

} // namespace Internal
} // namespace ClangTools
