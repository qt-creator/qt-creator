/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef COMMITDATA_H
#define COMMITDATA_H

#include <QtCore/QStringList>
#include <QtCore/QPair>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace Git {
namespace Internal {

// Read-only
struct GitSubmitEditorPanelInfo
{
    void clear();
    QString repository;
    QString description;
    QString branch;
};

QDebug operator<<(QDebug d, const GitSubmitEditorPanelInfo &);

struct GitSubmitEditorPanelData
{
    void clear();
    // Format as "John Doe <jdoe@foobar.com>"
    QString authorString() const;

    QString author;
    QString email;
};

QDebug operator<<(QDebug d, const GitSubmitEditorPanelData &);

struct CommitData
{
    // A pair of state string/file name ('modified', 'file.cpp').
    typedef QPair<QString, QString> StateFilePair;

    void clear();
    // Parse the files and the branch of panelInfo
    // from a git status output
    bool parseFilesFromStatus(const QString &output);

    bool filesEmpty() const;

    // Convenience to retrieve the file names from
    // the specification list. Optionally filter for a certain state
    QStringList stagedFileNames(const QString &stateFilter = QString()) const;
    QStringList unstagedFileNames(const QString &stateFilter = QString()) const;

    GitSubmitEditorPanelInfo panelInfo;
    GitSubmitEditorPanelData panelData;

    QList<StateFilePair> stagedFiles;
    QList<StateFilePair> unstagedFiles;
    QStringList untrackedFiles;
};

QDebug operator<<(QDebug d, const CommitData &);


} // namespace Internal
} // namespace Git

#endif // COMMITDATA_H
