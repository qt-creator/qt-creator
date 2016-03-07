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

#ifndef CLANGBACKEND_CODECOMPLETER_H
#define CLANGBACKEND_CODECOMPLETER_H

#include "clangtranslationunit.h"

#include <codecompletion.h>

#include <utf8stringvector.h>

namespace ClangBackEnd {

class ClangCodeCompleteResults;

class CodeCompleter
{
public:
    CodeCompleter() = default;
    CodeCompleter(TranslationUnit translationUnit);

    CodeCompletions complete(uint line, uint column);

    CompletionCorrection neededCorrection() const;

public: // for tests
    bool hasDotAt(uint line, uint column) const;

private:
    uint defaultOptions() const;

    ClangCodeCompleteResults complete(uint line,
                                      uint column,
                                      CXUnsavedFile *unsavedFiles,
                                      unsigned unsavedFileCount);

    ClangCodeCompleteResults completeWithArrowInsteadOfDot(uint line, uint column);

    Utf8String filePath() const;
    static void checkCodeCompleteResult(CXCodeCompleteResults *completeResults);

private:
    TranslationUnit translationUnit;
    CompletionCorrection neededCorrection_ = CompletionCorrection::NoCorrection;
};

} // namespace ClangBackEnd

#endif // CLANGBACKEND_CODECOMPLETER_H
