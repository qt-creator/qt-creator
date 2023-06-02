// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mercurialsettings.h"
#include <coreplugin/editormanager/ieditor.h>
#include <vcsbase/vcsbaseclient.h>

namespace VcsBase { class VcsBaseDiffEditorController; }

namespace Mercurial::Internal {

class MercurialDiffEditorController;

class MercurialClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    MercurialClient();

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

    void annotate(const Utils::FilePath &workingDir, const QString &file,
                  int lineNumber = -1, const QString &revision = {},
                  const QStringList &extraOptions = {}, int firstLine = -1) override;
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

    void view(const Utils::FilePath &source, const QString &id,
              const QStringList &extraOptions = QStringList()) override;

protected:
    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override;
    QStringList revisionSpec(const QString &revision) const override;
    StatusItem parseStatusLine(const QString &line) const override;

signals:
    void needUpdate();
    void needMerge();

private:
    void requestReload(const QString &documentId, const Utils::FilePath &source,
                       const QString &title, const Utils::FilePath &workingDirectory,
                       const QStringList &args);
    void parsePullOutput(const QString &output);
};

} // Mercurial::Internal
