/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CVSUTILS_H
#define CVSUTILS_H

#include "cvssubmiteditor.h"

#include <QString>
#include <QList>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Cvs {
namespace Internal {

// Utilities to parse output of a CVS log.

// A revision of a file.
struct CvsRevision
{
    CvsRevision(const QString &rev);

    QString revision;
    QString date; // ISO-Format (YYYY-MM-DD)
    QString commitId;
};

// A log entry consisting of the file and its revisions.
struct CvsLogEntry
{
    CvsLogEntry(const QString &file);

    QString file;
    QList<CvsRevision> revisions;
};

QDebug operator<<(QDebug d, const CvsLogEntry &);

// Parse. Pass on a directory to obtain full paths when
// running from the repository directory.
QList<CvsLogEntry> parseLogEntries(const QString &output,
                                   const QString &directory = QString(),
                                   const QString filterCommitId = QString());

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

} // namespace Internal
} // namespace Cvs

#endif // CVSUTILS_H
