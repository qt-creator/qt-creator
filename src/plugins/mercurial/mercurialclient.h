// Copyright (C) 2016 Brian McGillion
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

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
    void showDiffEditor(const Utils::FilePath &workingDir, const QStringList &files = {});
    void import(const Utils::FilePath &repositoryRoot, const QStringList &files,
                const QStringList &extraOptions = {}) override;
    void revertAll(const Utils::FilePath &workingDir, const QString &revision = {},
                   const QStringList &extraOptions = {}) override;

    bool isVcsDirectory(const Utils::FilePath &filePath) const;

    void view(const Utils::FilePath &source, const QString &id,
              const QStringList &extraOptions = QStringList()) override;
    void parsePullOutput(const QString &output);

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
};

MercurialClient &mercurialClient();

} // Mercurial::Internal
