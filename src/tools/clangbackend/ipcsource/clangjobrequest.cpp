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

#include <QFileInfo>

#include <ostream>

namespace ClangBackEnd {

#define RETURN_TEXT_FOR_CASE(enumValue) case JobRequest::Type::enumValue: return #enumValue
static const char *JobRequestTypeToText(JobRequest::Type type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(UpdateDocumentAnnotations);
        RETURN_TEXT_FOR_CASE(ParseSupportiveTranslationUnit);
        RETURN_TEXT_FOR_CASE(ReparseSupportiveTranslationUnit);
        RETURN_TEXT_FOR_CASE(CreateInitialDocumentPreamble);
        RETURN_TEXT_FOR_CASE(CompleteCode);
        RETURN_TEXT_FOR_CASE(RequestDocumentAnnotations);
        RETURN_TEXT_FOR_CASE(RequestReferences);
        RETURN_TEXT_FOR_CASE(SuspendDocument);
        RETURN_TEXT_FOR_CASE(ResumeDocument);
    }

    return "UnhandledJobRequestType";
}
#undef RETURN_TEXT_FOR_CASE

#define RETURN_TEXT_FOR_CASE(enumValue) case PreferredTranslationUnit::enumValue: return #enumValue
const char *preferredTranslationUnitToText(PreferredTranslationUnit type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(RecentlyParsed);
        RETURN_TEXT_FOR_CASE(PreviouslyParsed);
        RETURN_TEXT_FOR_CASE(LastUninitialized);
    }

    return "UnhandledPreferredTranslationUnitType";
}
#undef RETURN_TEXT_FOR_CASE

QDebug operator<<(QDebug debug, JobRequest::Type type)
{
    debug << JobRequestTypeToText(type);

    return debug;
}

std::ostream &operator<<(std::ostream &os, JobRequest::Type type)
{
    return os << JobRequestTypeToText(type);
}

std::ostream &operator<<(std::ostream &os, PreferredTranslationUnit preferredTranslationUnit)
{
    return os << preferredTranslationUnitToText(preferredTranslationUnit);
}

QDebug operator<<(QDebug debug, const JobRequest &jobRequest)
{
    debug.nospace() << "Job<"
                    << jobRequest.id
                    << ","
                    << QFileInfo(jobRequest.filePath).fileName()
                    << ","
                    << JobRequestTypeToText(jobRequest.type)
                    << ","
                    << preferredTranslationUnitToText(jobRequest.preferredTranslationUnit)
                    << ">";

    return debug.space();
}

JobRequest::JobRequest()
{
    static quint64 idCounter = 0;
    id = ++idCounter;
}

bool JobRequest::operator==(const JobRequest &other) const
{
    return type == other.type
        && expirationReasons == other.expirationReasons
        && conditions == other.conditions

        && filePath == other.filePath
        && projectPartId == other.projectPartId
        && unsavedFilesChangeTimePoint == other.unsavedFilesChangeTimePoint
        && projectChangeTimePoint == other.projectChangeTimePoint
        && documentRevision == other.documentRevision
        && preferredTranslationUnit == other.preferredTranslationUnit

        && line == other.line
        && column == other.column
        && ticketNumber == other.ticketNumber;
}

JobRequest::ExpirationReasons JobRequest::expirationReasonsForType(Type type)
{
    switch (type) {
    case Type::UpdateDocumentAnnotations:
        return ExpirationReasons(ExpirationReason::AnythingChanged);
    case Type::RequestReferences:
    case Type::RequestDocumentAnnotations:
        return ExpirationReasons(ExpirationReason::DocumentClosed)
             | ExpirationReasons(ExpirationReason::DocumentRevisionChanged);
    default:
        return ExpirationReason::DocumentClosed;
    }
}

JobRequest::Conditions JobRequest::conditionsForType(JobRequest::Type type)
{
    if (type == Type::SuspendDocument) {
        return Conditions(Condition::DocumentUnsuspended)
             | Conditions(Condition::DocumentNotVisible);
    }

    if (type == Type::ResumeDocument) {
        return Conditions(Condition::DocumentSuspended)
             | Conditions(Condition::DocumentVisible);
    }

    Conditions conditions = Conditions(Condition::DocumentUnsuspended)
                          | Conditions(Condition::DocumentVisible);

    if (type == Type::RequestReferences)
        conditions |= Condition::CurrentDocumentRevision;

    return conditions;
}

} // namespace ClangBackEnd
