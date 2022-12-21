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
    SubversionClient(SubversionSettings *settings);

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
             bool enableAnnotationContextMenu = false) override;

    void describe(const Utils::FilePath &workingDirectory, int changeNumber, const QString &title);

    // Add authorization options to the command line arguments.
    static QStringList addAuthenticationOptions(const SubversionSettings &settings);

    QString synchronousTopic(const Utils::FilePath &repository) const;

    static QString escapeFile(const QString &file);
    static QStringList escapeFiles(const QStringList &files);

protected:
    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override;

private:
    SubversionDiffEditorController *findOrCreateDiffEditor(const QString &documentId,
        const QString &source, const QString &title, const Utils::FilePath &workingDirectory);

    mutable Utils::FilePath m_svnVersionBinary;
    mutable QString m_svnVersion;
};

} // namespace Internal
} // namespace Subversion
