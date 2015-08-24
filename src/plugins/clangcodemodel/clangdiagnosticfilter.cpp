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
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangdiagnosticfilter.h"

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
    const auto isLocalWarning = [this] (const ClangBackEnd::DiagnosticContainer &diagnostic) {
        return isWarningOrNote(diagnostic.severity())
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

