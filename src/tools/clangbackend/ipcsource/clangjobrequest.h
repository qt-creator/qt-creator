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

#include <utf8string.h>

#include <QFlags>
#include <QDebug>
#include <QVector>

#include <chrono>

namespace ClangBackEnd {

using time_point = std::chrono::steady_clock::time_point;

class JobRequest
{
public:
    enum class Type {
        UpdateDocumentAnnotations,
        CreateInitialDocumentPreamble,
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

public:
    quint64 id = 0;
    Type type;
    Requirements requirements;

    // General
    Utf8String filePath;
    Utf8String projectPartId;
    time_point unsavedFilesChangeTimePoint;
    time_point projectChangeTimePoint;
    uint documentRevision = 0;

    // For code completion
    quint32 line = 0;
    quint32 column = 0;
    quint64 ticketNumber = 0;
};

using JobRequests = QVector<JobRequest>;

QDebug operator<<(QDebug debug, const JobRequest &jobRequest);

} // namespace ClangBackEnd
