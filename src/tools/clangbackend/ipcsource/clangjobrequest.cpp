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

#include "clangjobrequest.h"

namespace ClangBackEnd {

#define RETURN_TEXT_FOR_CASE(enumValue) case JobRequest::Type::enumValue: return #enumValue
static const char *JobRequestTypeToText(JobRequest::Type type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(UpdateDocumentAnnotations);
        RETURN_TEXT_FOR_CASE(CreateInitialDocumentPreamble);
        RETURN_TEXT_FOR_CASE(CompleteCode);
        RETURN_TEXT_FOR_CASE(RequestDocumentAnnotations);
    }

    return "UnhandledJobRequestType";
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, JobRequest::Type type)
{
    debug << JobRequestTypeToText(type);

    return debug;
}

QDebug operator<<(QDebug debug, const JobRequest &jobRequest)
{
    debug.nospace() << "Job<"
                    << jobRequest.id
                    << ","
                    << JobRequestTypeToText(jobRequest.type)
                    << ","
                    << jobRequest.filePath
                    << ">";

    return debug;
}

JobRequest::JobRequest()
{
    static quint64 idCounter = 0;
    id = ++idCounter;
}

JobRequest::Requirements JobRequest::requirementsForType(Type type)
{
    switch (type) {
    case JobRequest::Type::UpdateDocumentAnnotations:
        return JobRequest::Requirements(JobRequest::All);
    case JobRequest::Type::RequestDocumentAnnotations:
        return JobRequest::Requirements(JobRequest::DocumentValid
                                       |JobRequest::CurrentDocumentRevision);
    case JobRequest::Type::CompleteCode:
    case JobRequest::Type::CreateInitialDocumentPreamble:
        return JobRequest::Requirements(JobRequest::DocumentValid);
    }

    return JobRequest::Requirements(JobRequest::DocumentValid);
}

} // namespace ClangBackEnd
