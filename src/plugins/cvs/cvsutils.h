// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cvssubmiteditor.h"

#include <QString>
#include <QList>

namespace Cvs::Internal {

// Utilities to parse output of a CVS log.

// A revision of a file.
class CvsRevision
{
public:
    CvsRevision(const QString &rev);

    QString revision;
    QString date; // ISO-Format (YYYY-MM-DD)
    QString commitId;
};

// A log entry consisting of the file and its revisions.
class CvsLogEntry
{
public:
    CvsLogEntry(const QString &file);

    QString file;
    QList<CvsRevision> revisions;
};

// Parse. Pass on a directory to obtain full paths when
// running from the repository directory.
QList<CvsLogEntry> parseLogEntries(const QString &output,
                                   const QString &directory = QString(),
                                   const QString &filterCommitId = QString());

// Tortoise CVS outputs unknown files with question marks in
// the diff output on stdout ('? foo'); remove
QString fixDiffOutput(QString d);

// Parse the status output of CVS (stdout/stderr merged
// to catch directories).
typedef CvsSubmitEditor::StateFilePairs StateList;
StateList parseStatusOutput(const QString &directory, const QString &output);

// Revision number utilities: Decrement version number "1.2" -> "1.1"
QString previousRevision(const QString &rev);
// Revision number utilities: Is it "[1.2...].1"?
bool isFirstRevision(const QString &r);

} // Cvs::Internal
