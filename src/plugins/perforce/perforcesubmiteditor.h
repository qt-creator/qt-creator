// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QStringList>
#include <QMap>

namespace VcsBase { class SubmitFileModel; }

namespace Perforce::Internal {

class PerforceSubmitEditorWidget;
class PerforcePluginPrivate;

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
    PerforceSubmitEditor();

    /* The p4 submit starts with all opened files. Restrict
     * it to the current project files in question. */
    void restrictToProjectFiles(const QStringList &files);

    static QString fileFromChangeLine(const QString &line);

protected:
    QByteArray fileContents() const override;
    bool setFileContents(const QByteArray &contents) override;

private:
    inline PerforceSubmitEditorWidget *submitEditorWidget();
    bool parseText(QString text);
    void updateFields();
    void updateEntries();

    QMap<QString, QString> m_entries;
    VcsBase::SubmitFileModel *m_fileModel;
};

} // Perforce::Internal
