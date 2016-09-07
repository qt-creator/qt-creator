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

#pragma once

#include "clangtranslationunit.h"
#include "unsavedfiles.h"

#include <codecompletion.h>

#include <utf8stringvector.h>

namespace ClangBackEnd {

class ClangCodeCompleteResults;

class CodeCompleter
{
public:
    CodeCompleter() = default;
    CodeCompleter(const TranslationUnit &translationUnit,
                  const UnsavedFiles &unsavedFiles);

    CodeCompletions complete(uint line, uint column);

    CompletionCorrection neededCorrection() const;

private:
    uint defaultOptions() const;
    UnsavedFile &unsavedFile();

    void tryDotArrowCorrectionIfNoResults(ClangCodeCompleteResults &results,
                                          uint line,
                                          uint column);

    ClangCodeCompleteResults completeHelper(uint line, uint column);
    ClangCodeCompleteResults completeWithArrowInsteadOfDot(uint line,
                                                           uint column,
                                                           uint dotPosition);

private:
    TranslationUnit translationUnit;
    UnsavedFiles unsavedFiles;
    CompletionCorrection neededCorrection_ = CompletionCorrection::NoCorrection;
};

} // namespace ClangBackEnd
