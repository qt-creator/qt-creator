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

#include <QFlags>
#include <QDebug>
#include <QVector>

#include <functional>

namespace ClangBackEnd {

class Document;

class JobRequest
{
public:
    enum class Type {
        UpdateDocumentAnnotations,
        CreateInitialDocumentPreamble,

        ParseSupportiveTranslationUnit,
        ReparseSupportiveTranslationUnit,

        CompleteCode,
        RequestDocumentAnnotations,
    };

    enum Requirement {
        None                    = 1 << 0,

        DocumentValid           = 1 << 1,
        CurrentDocumentRevision = 1 << 3, // Only effective if DocumentValid is also set
        CurrentUnsavedFiles     = 1 << 2,
        CurrentProject          = 1 << 4,

        All = DocumentValid | CurrentUnsavedFiles | CurrentDocumentRevision | CurrentProject
    };
    Q_DECLARE_FLAGS(Requirements, Requirement)

public:
    static Requirements requirementsForType(Type type);

    JobRequest();

    bool operator==(const JobRequest &other) const;

public:
    quint64 id = 0;
    Type type;
    Requirements requirements;

    // General
    Utf8String filePath;
    Utf8String projectPartId;
    TimePoint unsavedFilesChangeTimePoint;
    TimePoint projectChangeTimePoint;
    uint documentRevision = 0;
    PreferredTranslationUnit preferredTranslationUnit = PreferredTranslationUnit::RecentlyParsed;

    // For code completion
    quint32 line = 0;
    quint32 column = 0;
    quint64 ticketNumber = 0;
};

using JobRequests = QVector<JobRequest>;
using JobRequestCreator = std::function<JobRequest(const Document &,
                                                   JobRequest::Type ,
                                                   PreferredTranslationUnit)>;

QDebug operator<<(QDebug debug, const JobRequest &jobRequest);

} // namespace ClangBackEnd
