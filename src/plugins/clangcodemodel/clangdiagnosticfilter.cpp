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

#include "clangdiagnosticfilter.h"

#include <cpptools/cppprojectfile.h>

#include <utf8stringvector.h>

namespace {

bool isWarningOrNote(ClangBackEnd::DiagnosticSeverity severity)
{
    using ClangBackEnd::DiagnosticSeverity;
    switch (severity) {
        case DiagnosticSeverity::Ignored:
        case DiagnosticSeverity::Note:
        case DiagnosticSeverity::Warning: return true;
        case DiagnosticSeverity::Error:
        case DiagnosticSeverity::Fatal: return false;
    }

    Q_UNREACHABLE();
}

bool isBlackListedDiagnostic(const ClangBackEnd::DiagnosticContainer &diagnostic,
                             bool isHeaderFile)
{
    static const Utf8StringVector blackList{
        Utf8StringLiteral("warning: #pragma once in main file"),
        Utf8StringLiteral("warning: #include_next in primary source file")
    };

    return isHeaderFile && blackList.contains(diagnostic.text());
}

template <class Condition>
QVector<ClangBackEnd::DiagnosticContainer>
filterDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                  const Condition &condition)
{
    QVector<ClangBackEnd::DiagnosticContainer> filteredDiagnostics;

    std::copy_if(diagnostics.cbegin(),
                 diagnostics.cend(),
                 std::back_inserter(filteredDiagnostics),
                 condition);

    return filteredDiagnostics;
}

template <class Condition>
void
filterDiagnostics(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics,
                  const Condition &condition,
                  QVector<ClangBackEnd::DiagnosticContainer> &filteredDiagnostic)
{
    std::copy_if(diagnostics.cbegin(),
                 diagnostics.cend(),
                 std::back_inserter(filteredDiagnostic),
                 condition);
}

} // anonymous namespace

namespace ClangCodeModel {
namespace Internal {

void ClangDiagnosticFilter::filterDocumentRelatedWarnings(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    using namespace CppTools;
    const bool isHeaderFile = ProjectFile::isHeader(ProjectFile::classify(m_filePath));

    const auto isLocalWarning = [this, isHeaderFile]
            (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return isWarningOrNote(diagnostic.severity())
            && !isBlackListedDiagnostic(diagnostic, isHeaderFile)
            && diagnostic.location().filePath() == m_filePath;
    };

    m_warningDiagnostics = filterDiagnostics(diagnostics, isLocalWarning);
}

void ClangDiagnosticFilter::filterDocumentRelatedErrors(
        const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    const auto isLocalWarning = [this] (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return !isWarningOrNote(diagnostic.severity())
            && diagnostic.location().filePath() == m_filePath;
    };

    m_errorDiagnostics = filterDiagnostics(diagnostics, isLocalWarning);
}

void ClangDiagnosticFilter::filterFixits()
{
    const auto hasFixIts = [] (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return diagnostic.fixIts().size() > 0;
    };

    m_fixItdiagnostics.clear();
    filterDiagnostics(m_warningDiagnostics, hasFixIts, m_fixItdiagnostics);
    filterDiagnostics(m_errorDiagnostics, hasFixIts, m_fixItdiagnostics);

    for (const auto &warningDiagnostic : m_warningDiagnostics)
        filterDiagnostics(warningDiagnostic.children(), hasFixIts, m_fixItdiagnostics);
    for (const auto &warningDiagnostic : m_errorDiagnostics)
        filterDiagnostics(warningDiagnostic.children(), hasFixIts, m_fixItdiagnostics);
}

ClangDiagnosticFilter::ClangDiagnosticFilter(const QString &filePath)
    : m_filePath(filePath)
{
}

void ClangDiagnosticFilter::filter(const QVector<ClangBackEnd::DiagnosticContainer> &diagnostics)
{
    filterDocumentRelatedWarnings(diagnostics);
    filterDocumentRelatedErrors(diagnostics);
    filterFixits();
}

QVector<ClangBackEnd::DiagnosticContainer> ClangDiagnosticFilter::takeWarnings()
{
    auto diagnostics = m_warningDiagnostics;
    m_warningDiagnostics.clear();

    return diagnostics;
}

QVector<ClangBackEnd::DiagnosticContainer> ClangDiagnosticFilter::takeErrors()
{
    auto diagnostics = m_errorDiagnostics;
    m_errorDiagnostics.clear();

    return diagnostics;
}

QVector<ClangBackEnd::DiagnosticContainer> ClangDiagnosticFilter::takeFixIts()
{
    auto diagnostics = m_fixItdiagnostics;
    m_fixItdiagnostics.clear();

    return diagnostics;
}

} // namespace Internal
} // namespace ClangCodeModel

