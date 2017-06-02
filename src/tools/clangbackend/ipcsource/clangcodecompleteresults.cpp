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

#include "clangcodecompleteresults.h"

#include <memory>

namespace ClangBackEnd {

using std::swap;

ClangCodeCompleteResults::ClangCodeCompleteResults(CXCodeCompleteResults *cxCodeCompleteResults)
    : cxCodeCompleteResults(cxCodeCompleteResults)
{
}

ClangCodeCompleteResults::~ClangCodeCompleteResults()
{
    clang_disposeCodeCompleteResults(cxCodeCompleteResults);
}

bool ClangCodeCompleteResults::isNull() const
{
    return cxCodeCompleteResults == nullptr;
}

bool ClangCodeCompleteResults::isEmpty() const
{
    return cxCodeCompleteResults->NumResults == 0;
}

bool ClangCodeCompleteResults::hasResults() const
{
    return !isNull() && !isEmpty();
}

bool ClangCodeCompleteResults::hasNoResultsForDotCompletion() const
{
    return !hasResults() && isDotCompletion();
}

bool ClangCodeCompleteResults::hasUnknownContext() const
{
    const unsigned long long contexts = clang_codeCompleteGetContexts(cxCodeCompleteResults);
    return contexts == CXCompletionContext_Unknown;
}

bool ClangCodeCompleteResults::isDotCompletion() const
{
    const unsigned long long contexts = clang_codeCompleteGetContexts(cxCodeCompleteResults);

    return contexts & CXCompletionContext_DotMemberAccess;
}

CXCodeCompleteResults *ClangCodeCompleteResults::data() const
{
    return cxCodeCompleteResults;
}

ClangCodeCompleteResults &ClangCodeCompleteResults::operator=(ClangCodeCompleteResults &&clangCodeCompleteResults)
{
    swap(cxCodeCompleteResults, clangCodeCompleteResults.cxCodeCompleteResults);

    return *this;
}

ClangCodeCompleteResults::ClangCodeCompleteResults(ClangCodeCompleteResults &&clangCodeCompleteResults)
{
    swap(cxCodeCompleteResults, clangCodeCompleteResults.cxCodeCompleteResults);
}

} // namespace ClangBackEnd

