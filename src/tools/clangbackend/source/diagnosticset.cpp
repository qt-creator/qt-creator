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

#include "diagnosticset.h"

#include "diagnostic.h"

#include <diagnosticcontainer.h>

#include <memory>

namespace ClangBackEnd {

DiagnosticSet::DiagnosticSet(CXDiagnosticSet cxDiagnosticSet)
    : cxDiagnosticSet(cxDiagnosticSet)
{
}

DiagnosticSet::~DiagnosticSet()
{
    clang_disposeDiagnosticSet(cxDiagnosticSet);
}

DiagnosticSet::DiagnosticSet(DiagnosticSet &&other)
    : cxDiagnosticSet(std::move(other.cxDiagnosticSet))
{
    other.cxDiagnosticSet = nullptr;
}

DiagnosticSet &DiagnosticSet::operator=(DiagnosticSet &&other)
{
    if (this != &other) {
        clang_disposeDiagnosticSet(cxDiagnosticSet);
        cxDiagnosticSet = std::move(other.cxDiagnosticSet);
        other.cxDiagnosticSet = nullptr;
    }

    return *this;
}

Diagnostic DiagnosticSet::front() const
{
    return Diagnostic(clang_getDiagnosticInSet(cxDiagnosticSet, 0));
}

Diagnostic DiagnosticSet::back() const
{
    return Diagnostic(clang_getDiagnosticInSet(cxDiagnosticSet, size() - 1));
}

DiagnosticSet::ConstIterator DiagnosticSet::begin() const
{
    return DiagnosticSetIterator(cxDiagnosticSet, 0);
}

DiagnosticSet::ConstIterator DiagnosticSet::end() const
{
    return DiagnosticSetIterator(cxDiagnosticSet, size());
}

QVector<DiagnosticContainer> DiagnosticSet::toDiagnosticContainers() const
{
    const auto isAcceptedDiagnostic = [](const Diagnostic &) { return true; };

    return toDiagnosticContainers(isAcceptedDiagnostic);
}

QVector<DiagnosticContainer> DiagnosticSet::toDiagnosticContainers(
        const IsAcceptedDiagnostic &isAcceptedDiagnostic) const
{
    QVector<DiagnosticContainer> diagnosticContainers;
    diagnosticContainers.reserve(size());

    for (const Diagnostic &diagnostic : *this) {
        if (isAcceptedDiagnostic(diagnostic))
            diagnosticContainers.push_back(diagnostic.toDiagnosticContainer());
    }

    return diagnosticContainers;
}

uint DiagnosticSet::size() const
{
    return clang_getNumDiagnosticsInSet(cxDiagnosticSet);
}

bool DiagnosticSet::isNull() const
{
    return cxDiagnosticSet == nullptr;
}

Diagnostic DiagnosticSet::at(uint index) const
{
    return Diagnostic(clang_getDiagnosticInSet(cxDiagnosticSet, index));
}

} // namespace ClangBackEnd

