// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <vcsbase/vcsbaseclient.h>

#include <utils/fileutils.h>

namespace Subversion {
namespace Internal {

class SubversionDiffEditorController;
class SubversionSettings;

class SubversionClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    SubversionClient();

    bool doCommit(const Utils::FilePath &repositoryRoot,
                  const QStringList &files,
                  const QString &commitMessageFile,
                  const QStringList &extraOptions = {}) const;
    void commit(const Utils::FilePath &repositoryRoot,
                const QStringList &files,
                const QString &commitMessageFile,
                const QStringList &extraOptions = {}) override;

    void diff(const Utils::FilePath &workingDirectory,
              const QStringList &files,
              const QStringList &extraOptions) override;

    void log(const Utils::FilePath &workingDir,
             const QStringList &files = {},
             const QStringList &extraOptions = {},
             bool enableAnnotationContextMenu = false,
             const std::function<void(Utils::CommandLine &)> &addAuthOptions = {}) override;

    void describe(const Utils::FilePath &workingDirectory, int changeNumber, const QString &title);

    // Add authentication options to the command line arguments.
    struct AddAuthOptions {};

    QString synchronousTopic(const Utils::FilePath &repository) const;

    static QString escapeFile(const QString &file);
    static QStringList escapeFiles(const QStringList &files);

protected:
    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override;

private:
    SubversionDiffEditorController *findOrCreateDiffEditor(const QString &documentId,
        const Utils::FilePath &source, const QString &title,
        const Utils::FilePath &workingDirectory);

    mutable Utils::FilePath m_svnVersionBinary;
    mutable QString m_svnVersion;
};

Utils::CommandLine &operator<<(Utils::CommandLine &command, SubversionClient::AddAuthOptions);

} // namespace Internal
} // namespace Subversion
