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

#include "clangbackend_global.h"
#include "clangclock.h"

#include <utf8string.h>
#include <utf8stringvector.h>

#include <QFlags>
#include <QDebug>
#include <QVector>

#include <functional>

namespace ClangBackEnd {

class ClangCodeModelClientInterface;
class Document;
class IAsyncJob;

class JobRequest
{
public:
    enum class Type {
        Invalid,

        UpdateDocumentAnnotations,
        CreateInitialDocumentPreamble,

        ParseSupportiveTranslationUnit,
        ReparseSupportiveTranslationUnit,

        CompleteCode,
        RequestDocumentAnnotations,
        RequestReferences,
        FollowSymbol,

        SuspendDocument,
        ResumeDocument,
    };

    enum class RunCondition {
        NoCondition             = 1 << 0,
        DocumentVisible         = 1 << 1,
        DocumentNotVisible      = 1 << 2,
        DocumentSuspended       = 1 << 3,
        DocumentUnsuspended     = 1 << 4,
        CurrentDocumentRevision = 1 << 5,
    };
    Q_DECLARE_FLAGS(RunConditions, RunCondition)

    enum class ExpirationCondition {
        Never                   = 1 << 0,

        DocumentClosed          = 1 << 1,
        DocumentRevisionChanged = 1 << 2, // Only effective if DocumentIsClosed is also set
        UnsavedFilesChanged     = 1 << 3,
        ProjectChanged          = 1 << 4,

        AnythingChanged = DocumentClosed
                        | DocumentRevisionChanged
                        | UnsavedFilesChanged
                        | ProjectChanged,
    };
    Q_DECLARE_FLAGS(ExpirationConditions, ExpirationCondition)

public:
    JobRequest(Type type = Type::Invalid);

    IAsyncJob *createJob() const;
    void cancelJob(ClangCodeModelClientInterface &client) const;

    bool operator==(const JobRequest &other) const;

public:
    quint64 id = 0;
    Type type;
    ExpirationConditions expirationConditions;
    RunConditions runConditions;

    // General
    Utf8String filePath;
    Utf8String projectPartId;
    TimePoint unsavedFilesChangeTimePoint;
    TimePoint projectChangeTimePoint;
    uint documentRevision = 0;
    PreferredTranslationUnit preferredTranslationUnit = PreferredTranslationUnit::RecentlyParsed;

    // Specific to some jobs
    quint32 line = 0;
    quint32 column = 0;
    qint32 funcNameStartLine = -1;
    qint32 funcNameStartColumn = -1;
    quint64 ticketNumber = 0;
    Utf8StringVector dependentFiles;
};

using JobRequests = QVector<JobRequest>;

QDebug operator<<(QDebug debug, const JobRequest &jobRequest);
std::ostream &operator<<(std::ostream &os, JobRequest::Type type);
std::ostream &operator<<(std::ostream &os, PreferredTranslationUnit preferredTranslationUnit);


} // namespace ClangBackEnd
