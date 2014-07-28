/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef DIFFEDITORCONTROLLER_H
#define DIFFEDITORCONTROLLER_H

#include "diffeditor_global.h"
#include "diffutils.h"

#include <QObject>

namespace DiffEditor {

class DiffEditorReloader;

class DIFFEDITOR_EXPORT DiffEditorController : public QObject
{
    Q_OBJECT
public:
    DiffEditorController(QObject *parent = 0);
    ~DiffEditorController();

    QString clearMessage() const;

    QList<FileData> diffFiles() const;
    QString workingDirectory() const;
    QString description() const;
    bool isDescriptionEnabled() const;
    int contextLinesNumber() const;
    bool isIgnoreWhitespace() const;

    QString makePatch(bool revert, bool addPrefix = false) const;

    DiffEditorReloader *reloader() const;
    void setReloader(DiffEditorReloader *reloader);

public slots:
    void clear();
    void clear(const QString &message);
    void setDiffFiles(const QList<FileData> &diffFileList,
                      const QString &workingDirectory = QString());
    void setDescription(const QString &description);
    void setDescriptionEnabled(bool on);
    void setContextLinesNumber(int lines);
    void setIgnoreWhitespace(bool ignore);
    void requestReload();
    void requestChunkActions(QMenu *menu,
                             int diffFileIndex,
                             int chunkIndex);
    void requestSaveState();
    void requestRestoreState();
    void branchesForCommitReceived(const QString &output);
    void expandBranchesRequested();

signals:
    void cleared(const QString &message);
    void diffFilesChanged(const QList<FileData> &diffFileList,
                          const QString &workingDirectory);
    void descriptionChanged(const QString &description);
    void descriptionEnablementChanged(bool on);
    void contextLinesNumberChanged(int lines);
    void ignoreWhitespaceChanged(bool ignore);
    void chunkActionsRequested(QMenu *menu, bool isValid);
    void saveStateRequested();
    void restoreStateRequested();
    void expandBranchesRequested(const QString &revision);
    void reloaderChanged(DiffEditorReloader *reloader);

private:
    QString prepareBranchesForCommit(const QString &output);
    QString m_clearMessage;

    QList<FileData> m_diffFiles;
    int m_diffFileIndex;
    int m_chunkIndex;
    QString m_workingDirectory;
    QString m_description;
    bool m_descriptionEnabled;
    int m_contextLinesNumber;
    bool m_ignoreWhitespace;
    DiffEditorReloader *m_reloader;
};

} // namespace DiffEditor

#endif // DIFFEDITORCONTROLLER_H
