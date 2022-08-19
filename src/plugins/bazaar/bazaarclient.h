// Copyright (C) 2016 Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "bazaarsettings.h"
#include "branchinfo.h"
#include <vcsbase/vcsbaseclient.h>

namespace Bazaar {
namespace Internal {

class BazaarSettings;

class BazaarClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    explicit BazaarClient(BazaarSettings *settings);

    BranchInfo synchronousBranchQuery(const Utils::FilePath &repositoryRoot) const;
    bool synchronousUncommit(const Utils::FilePath &workingDir,
                             const QString &revision = {},
                             const QStringList &extraOptions = {});
    void commit(const Utils::FilePath &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile, const QStringList &extraOptions = {}) override;
    VcsBase::VcsBaseEditorWidget *annotate(
            const Utils::FilePath &workingDir, const QString &file, const QString &revision = {},
            int lineNumber = -1, const QStringList &extraOptions ={}) override;
    bool isVcsDirectory(const Utils::FilePath &filePath) const;
    Utils::FilePath findTopLevelForFile(const Utils::FilePath &file) const override;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const;
    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = {}) override;

    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override;
    QString vcsCommandString(VcsCommandTag cmd) const override;
    Utils::ExitCodeInterpreter exitCodeInterpreter(VcsCommandTag cmd) const override;
    QStringList revisionSpec(const QString &revision) const override;
    StatusItem parseStatusLine(const QString &line) const override;

private:
    friend class CloneWizard;
};

} // namespace Internal
} // namespace Bazaar
