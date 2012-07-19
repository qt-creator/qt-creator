/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef PERFORCESUBMITEDITOR_H
#define PERFORCESUBMITEDITOR_H

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QStringList>
#include <QMap>

namespace VcsBase {
    class SubmitFileModel;
}

namespace Perforce {
namespace Internal {

class PerforceSubmitEditorWidget;
class PerforcePlugin;

/* PerforceSubmitEditor: In p4, the file list is contained in the
 * submit message file (change list). On setting the file contents,
 * it is split apart in message and file list and re-assembled
 * when retrieving the file list.
 * As a p4 submit starts with all opened files, there is API to restrict
 * the file list to current project files in question
 * (restrictToProjectFiles()). */
class PerforceSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT

public:
    explicit PerforceSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters, QWidget *parent);

    /* The p4 submit starts with all opened files. Restrict
     * it to the current project files in question. */
    void restrictToProjectFiles(const QStringList &files);

    static QString fileFromChangeLine(const QString &line);

protected:
    QByteArray fileContents() const;
    bool setFileContents(const QString &contents);

private:
    inline PerforceSubmitEditorWidget *submitEditorWidget();
    bool parseText(QString text);
    void updateFields();
    void updateEntries();

    QMap<QString, QString> m_entries;
    VcsBase::SubmitFileModel *m_fileModel;
};

} // namespace Internal
} // namespace Perforce

#endif // PERFORCESUBMITEDITOR_H
