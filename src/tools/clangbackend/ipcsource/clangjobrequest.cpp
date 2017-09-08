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

#include "clangcompletecodejob.h"
#include "clangcreateinitialdocumentpreamblejob.h"
#include "clangfollowsymboljob.h"
#include "clangparsesupportivetranslationunitjob.h"
#include "clangreparsesupportivetranslationunitjob.h"
#include "clangrequestdocumentannotationsjob.h"
#include "clangrequestreferencesjob.h"
#include "clangresumedocumentjob.h"
#include "clangsuspenddocumentjob.h"
#include "clangupdatedocumentannotationsjob.h"

#include <clangsupport/clangcodemodelclientinterface.h>
#include <clangsupport/cmbcodecompletedmessage.h>
#include <clangsupport/followsymbolmessage.h>
#include <clangsupport/referencesmessage.h>

#include <utils/qtcassert.h>

#include <QFileInfo>

#include <ostream>

namespace ClangBackEnd {

#define RETURN_TEXT_FOR_CASE(enumValue) case JobRequest::Type::enumValue: return #enumValue
static const char *JobRequestTypeToText(JobRequest::Type type)
{
    switch (type) {
        RETURN_TEXT_FOR_CASE(Invalid);
        RETURN_TEXT_FOR_CASE(UpdateDocumentAnnotations);
        RETURN_TEXT_FOR_CASE(ParseSupportiveTranslationUnit);
        RETURN_TEXT_FOR_CASE(ReparseSupportiveTranslationUnit);
        RETURN_TEXT_FOR_CASE(CreateInitialDocumentPreamble);
        RETURN_TEXT_FOR_CASE(CompleteCode);
        RETURN_TEXT_FOR_CASE(RequestDocumentAnnotations);
        RETURN_TEXT_FOR_CASE(RequestReferences);
        RETURN_TEXT_FOR_CASE(FollowSymbol);
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

static JobRequest::ExpirationConditions expirationConditionsForType(JobRequest::Type type)
{
    using Type = JobRequest::Type;
    using Condition = JobRequest::ExpirationCondition;
    using Conditions = JobRequest::ExpirationConditions;

    switch (type) {
    case Type::UpdateDocumentAnnotations:
        return Conditions(Condition::AnythingChanged);
    case Type::RequestReferences:
    case Type::RequestDocumentAnnotations:
    case Type::FollowSymbol:
        return Conditions(Condition::DocumentClosed)
             | Conditions(Condition::DocumentRevisionChanged);
    default:
        return Condition::DocumentClosed;
    }
}

static JobRequest::RunConditions conditionsForType(JobRequest::Type type)
{
    using Type = JobRequest::Type;
    using Condition = JobRequest::RunCondition;
    using Conditions = JobRequest::RunConditions;

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

JobRequest::JobRequest(Type type)
{
    static quint64 idCounter = 0;

    id = ++idCounter;
    this->type = type;
    runConditions = conditionsForType(type);
    expirationConditions = expirationConditionsForType(type);
}

IAsyncJob *JobRequest::createJob() const
{
    switch (type) {
    case JobRequest::Type::Invalid:
        QTC_CHECK(false && "Cannot create job for invalid job request.");
        break;
    case JobRequest::Type::UpdateDocumentAnnotations:
        return new UpdateDocumentAnnotationsJob();
    case JobRequest::Type::ParseSupportiveTranslationUnit:
        return new ParseSupportiveTranslationUnitJob();
    case JobRequest::Type::ReparseSupportiveTranslationUnit:
        return new ReparseSupportiveTranslationUnitJob();
    case JobRequest::Type::CreateInitialDocumentPreamble:
        return new CreateInitialDocumentPreambleJob();
    case JobRequest::Type::CompleteCode:
        return new CompleteCodeJob();
    case JobRequest::Type::RequestDocumentAnnotations:
        return new RequestDocumentAnnotationsJob();
    case JobRequest::Type::RequestReferences:
        return new RequestReferencesJob();
    case JobRequest::Type::FollowSymbol:
        return new FollowSymbolJob();
    case JobRequest::Type::SuspendDocument:
        return new SuspendDocumentJob();
    case JobRequest::Type::ResumeDocument:
        return new ResumeDocumentJob();
    }

    return nullptr;
}

void JobRequest::cancelJob(ClangCodeModelClientInterface &client) const
{
    // If a job request with a ticket number is cancelled, the plugin side
    // must get back some results in order to clean up the state there.

    switch (type) {
    case JobRequest::Type::Invalid:
    case JobRequest::Type::UpdateDocumentAnnotations:
    case JobRequest::Type::ParseSupportiveTranslationUnit:
    case JobRequest::Type::ReparseSupportiveTranslationUnit:
    case JobRequest::Type::CreateInitialDocumentPreamble:
    case JobRequest::Type::RequestDocumentAnnotations:
    case JobRequest::Type::SuspendDocument:
    case JobRequest::Type::ResumeDocument:
        break;
    case JobRequest::Type::RequestReferences:
        client.references(ReferencesMessage(FileContainer(),
                                            QVector<SourceRangeContainer>(),
                                            false,
                                            ticketNumber));
        break;
    case JobRequest::Type::CompleteCode:
        client.codeCompleted(CodeCompletedMessage(CodeCompletions(),
                                                  CompletionCorrection::NoCorrection,
                                                  ticketNumber));
        break;
    case JobRequest::Type::FollowSymbol:
        client.followSymbol(FollowSymbolMessage(FileContainer(),
                                                SourceRangeContainer(),
                                                ticketNumber));
        break;
    }
}

bool JobRequest::operator==(const JobRequest &other) const
{
    return type == other.type
        && expirationConditions == other.expirationConditions
        && runConditions == other.runConditions

        && filePath == other.filePath
        && projectPartId == other.projectPartId
        && unsavedFilesChangeTimePoint == other.unsavedFilesChangeTimePoint
        && projectChangeTimePoint == other.projectChangeTimePoint
        && documentRevision == other.documentRevision
        && preferredTranslationUnit == other.preferredTranslationUnit

        && line == other.line
        && column == other.column
        && ticketNumber == other.ticketNumber;

        // Additional members that are not compared here explicitly are
        // supposed to depend on the already compared ones.
}

} // namespace ClangBackEnd
