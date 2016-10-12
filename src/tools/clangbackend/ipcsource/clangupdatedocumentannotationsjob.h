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

#include "clangasyncjob.h"
#include "clangdocument.h"
#include "clangtranslationunitupdater.h"

#include <clangbackendipc/diagnosticcontainer.h>
#include <clangbackendipc/highlightingmarkcontainer.h>
#include <clangbackendipc/sourcerangecontainer.h>

namespace ClangBackEnd {

struct UpdateDocumentAnnotationsJobResult
{
    TranslationUnitUpdateResult updateResult;

    ClangBackEnd::DiagnosticContainer firstHeaderErrorDiagnostic;
    QVector<ClangBackEnd::DiagnosticContainer> diagnostics;
    QVector<HighlightingMarkContainer> highlightingMarks;
    QVector<SourceRangeContainer> skippedSourceRanges;
};

class UpdateDocumentAnnotationsJob : public AsyncJob<UpdateDocumentAnnotationsJobResult>
{
public:
    using AsyncResult = UpdateDocumentAnnotationsJobResult;

    AsyncPrepareResult prepareAsyncRun() override;
    void finalizeAsyncRun() override;

private:
    void incorporateUpdaterResult(const AsyncResult &result);
    void sendAnnotations(const AsyncResult &result);

private:
    Document m_pinnedDocument;
    FileContainer m_pinnedFileContainer;
};

} // namespace ClangBackEnd
