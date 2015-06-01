/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef GITSUBMITEDITOR_H
#define GITSUBMITEDITOR_H

#include "gitsettings.h" // CommitType

#include <vcsbase/vcsbasesubmiteditor.h>

#include <QStringList>

namespace VcsBase { class SubmitFileModel; }

namespace Git {
namespace Internal {

class GitSubmitEditorWidget;
class CommitData;
class CommitDataFetcher;
struct GitSubmitEditorPanelData;

class GitSubmitEditor : public VcsBase::VcsBaseSubmitEditor
{
    Q_OBJECT
public:
    explicit GitSubmitEditor(const VcsBase::VcsBaseSubmitEditorParameters *parameters);
    ~GitSubmitEditor() override;

    void setCommitData(const CommitData &);
    GitSubmitEditorPanelData panelData() const;
    CommitType commitType() const { return m_commitType; }
    QString amendSHA1() const;

signals:
    void diff(const QStringList &unstagedFiles, const QStringList &stagedFiles);
    void merge(const QStringList &unmergedFiles);
    void show(const QString &workingDirectory, const QString &commit);

protected:
    QByteArray fileContents() const override;
    void updateFileModel() override;

private slots:
    void slotDiffSelected(const QList<int> &rows);
    void showCommit(const QString &commit);
    void commitDataRetrieved(bool success);

private:
    inline GitSubmitEditorWidget *submitEditorWidget();
    inline const GitSubmitEditorWidget *submitEditorWidget() const;
    void resetCommitDataFetcher();

    VcsBase::SubmitFileModel *m_model;
    QTextCodec *m_commitEncoding;
    CommitType m_commitType;
    QString m_amendSHA1;
    QString m_workingDirectory;
    bool m_firstUpdate;
    CommitDataFetcher *m_commitDataFetcher;
};

} // namespace Internal
} // namespace Git

#endif // GITSUBMITEDITOR_H
