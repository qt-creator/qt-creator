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

#include "cppcompletionassistprocessor.h"

#include <cppeditor/cppeditorconstants.h>

namespace CppTools {

CppCompletionAssistProcessor::CppCompletionAssistProcessor()
    : m_positionForProposal(-1)
    , m_preprocessorCompletions(QStringList()
          << QLatin1String("define")
          << QLatin1String("error")
          << QLatin1String("include")
          << QLatin1String("line")
          << QLatin1String("pragma")
          << QLatin1String("pragma once")
          << QLatin1String("pragma omp atomic")
          << QLatin1String("pragma omp parallel")
          << QLatin1String("pragma omp for")
          << QLatin1String("pragma omp ordered")
          << QLatin1String("pragma omp parallel for")
          << QLatin1String("pragma omp section")
          << QLatin1String("pragma omp sections")
          << QLatin1String("pragma omp parallel sections")
          << QLatin1String("pragma omp single")
          << QLatin1String("pragma omp master")
          << QLatin1String("pragma omp critical")
          << QLatin1String("pragma omp barrier")
          << QLatin1String("pragma omp flush")
          << QLatin1String("pragma omp threadprivate")
          << QLatin1String("undef")
          << QLatin1String("if")
          << QLatin1String("ifdef")
          << QLatin1String("ifndef")
          << QLatin1String("elif")
          << QLatin1String("else")
          << QLatin1String("endif"))
    , m_hintProposal(0)
    , m_snippetCollector(QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID),
                         QIcon(QLatin1String(":/texteditor/images/snippet.png")))
{
}

void CppCompletionAssistProcessor::addSnippets()
{
    m_completions.append(m_snippetCollector.collect());
}

} // namespace CppTools
