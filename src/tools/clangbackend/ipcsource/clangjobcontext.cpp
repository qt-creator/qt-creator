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

#include "clangiasyncjob.h"

#include "translationunits.h"

namespace ClangBackEnd {

JobContext::JobContext(const JobRequest &jobRequest,
                       TranslationUnits *translationUnits,
                       UnsavedFiles *unsavedFiles,
                       ClangCodeModelClientInterface *clientInterface)
    : jobRequest(jobRequest)
    , translationUnits(translationUnits)
    , unsavedFiles(unsavedFiles)
    , client(clientInterface)
{
}

TranslationUnit JobContext::translationUnitForJobRequest() const
{
    return translationUnits->translationUnit(jobRequest.filePath, jobRequest.projectPartId);
}

bool JobContext::isOutdated() const
{
    return !isDocumentOpen() || documentRevisionChanged();
}

bool JobContext::isDocumentOpen() const
{
    const bool hasTranslationUnit
            = translationUnits->hasTranslationUnit(jobRequest.filePath, jobRequest.projectPartId);

    if (!hasTranslationUnit)
        qCDebug(jobsLog) << "Document already closed for results of" << jobRequest;

    return hasTranslationUnit;
}

bool JobContext::documentRevisionChanged() const
{
    const TranslationUnit &translationUnit
            = translationUnits->translationUnit(jobRequest.filePath, jobRequest.projectPartId);
    const bool revisionChanged = translationUnit.documentRevision() != jobRequest.documentRevision;

    if (revisionChanged)
        qCDebug(jobsLog) << "Document revision changed for results of" << jobRequest;

    return revisionChanged;
}

} // ClangBackEnd
