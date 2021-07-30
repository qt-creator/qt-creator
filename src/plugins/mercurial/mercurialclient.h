/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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

#include "mercurialsettings.h"
#include <coreplugin/editormanager/ieditor.h>
#include <vcsbase/vcsbaseclient.h>

namespace VcsBase { class VcsBaseDiffEditorController; }

namespace Mercurial {
namespace Internal {

class MercurialDiffEditorController;

class MercurialClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT
public:
    explicit MercurialClient(MercurialSettings *settings);

    bool synchronousClone(const Utils::FilePath &workingDir,
                          const QString &srcLocation,
                          const QString &dstLocation,
                          const QStringList &extraOptions = {}) override;
    bool synchronousPull(const Utils::FilePath &workingDir,
                         const QString &srcLocation,
                         const QStringList &extraOptions = {}) override;
    bool manifestSync(const Utils::FilePath &repository, const QString &filename);
    QString branchQuerySync(const QString &repositoryRoot);
    QStringList parentRevisionsSync(const Utils::FilePath &workingDirectory,
                             const QString &file /* = QString() */,
                             const QString &revision);
    QString shortDescriptionSync(const Utils::FilePath &workingDirectory, const QString &revision,
                              const QString &format /* = QString() */);
    QString shortDescriptionSync(const Utils::FilePath &workingDirectory, const QString &revision);
    void incoming(const Utils::FilePath &repositoryRoot, const QString &repository = {});
    void outgoing(const Utils::FilePath &repositoryRoot);
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const;

    VcsBase::VcsBaseEditorWidget *annotate(
            const Utils::FilePath &workingDir, const QString &file, const QString &revision = {},
            int lineNumber = -1, const QStringList &extraOptions = {}) override;
    void commit(const Utils::FilePath &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile,
                const QStringList &extraOptions = QStringList()) override;
    void diff(const Utils::FilePath &workingDir, const QStringList &files = {},
              const QStringList &extraOptions = {}) override;
    void import(const Utils::FilePath &repositoryRoot, const QStringList &files,
                const QStringList &extraOptions = {}) override;
    void revertAll(const Utils::FilePath &workingDir, const QString &revision = {},
                   const QStringList &extraOptions = {}) override;

    bool isVcsDirectory(const Utils::FilePath &filePath) const;
    Utils::FilePath findTopLevelForFile(const Utils::FilePath &file) const override;

    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = QStringList()) override;

protected:
    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override;
    QStringList revisionSpec(const QString &revision) const override;
    StatusItem parseStatusLine(const QString &line) const override;

signals:
    void needUpdate();
    void needMerge();

private:
    void requestReload(const QString &documentId, const QString &source, const QString &title,
                       const QString &workingDirectory,
                       const QStringList &args);
    void parsePullOutput(const QString &output);
};

} //namespace Internal
} //namespace Mercurial
