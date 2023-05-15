// Copyright (c) 2018 Artur Shepilko
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "fossilsettings.h"
#include "branchinfo.h"
#include "revisioninfo.h"

#include <vcsbase/vcsbaseclient.h>

#include <QList>

namespace Fossil {
namespace Internal {

class FossilSettings;
class FossilPluginPrivate;

class FossilClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT
public:
    enum SupportedFeature {
        AnnotateBlameFeature = 0x2,
        TimelineWidthFeature = 0x4,
        DiffIgnoreWhiteSpaceFeature = 0x8,
        TimelinePathFeature = 0x10,
        AnnotateRevisionFeature = 0x20,
        InfoHashFeature = 0x40,
        AllSupportedFeatures =  // | all defined features
            AnnotateBlameFeature
            | TimelineWidthFeature
            | DiffIgnoreWhiteSpaceFeature
            | TimelinePathFeature
            | AnnotateRevisionFeature
            | InfoHashFeature
    };
    Q_DECLARE_FLAGS(SupportedFeatures, SupportedFeature)

    static unsigned makeVersionNumber(int major, int minor, int patch);
    static QString makeVersionString(unsigned version);

    FossilClient();
    FossilSettings &settings() const;

    unsigned int synchronousBinaryVersion() const;
    BranchInfo synchronousCurrentBranch(const Utils::FilePath &workingDirectory);
    QList<BranchInfo> synchronousBranchQuery(const Utils::FilePath &workingDirectory);
    RevisionInfo synchronousRevisionQuery(const Utils::FilePath &workingDirectory,
                                          const QString &id = {}, bool getCommentMsg = false) const;
    QStringList synchronousTagQuery(const Utils::FilePath &workingDirectory, const QString &id = {});
    RepositorySettings synchronousSettingsQuery(const Utils::FilePath &workingDirectory);
    bool synchronousSetSetting(const Utils::FilePath &workingDirectory, const QString &property,
                               const QString &value = {}, bool isGlobal = false);
    bool synchronousConfigureRepository(const Utils::FilePath &workingDirectory,
                                        const RepositorySettings &newSettings,
                                        const RepositorySettings &currentSettings = {});
    QString synchronousUserDefaultQuery(const Utils::FilePath &workingDirectory);
    bool synchronousSetUserDefault(const Utils::FilePath &workingDirectory, const QString &userName);
    QString synchronousGetRepositoryURL(const Utils::FilePath &workingDirectory);
    QString synchronousTopic(const Utils::FilePath &workingDirectory);
    bool synchronousCreateRepository(const Utils::FilePath &workingDirectory,
                                     const QStringList &extraOptions = {}) final;
    bool synchronousMove(const Utils::FilePath &workingDir, const QString &from, const QString &to,
                         const QStringList &extraOptions = {}) final;
    bool synchronousPull(const Utils::FilePath &workingDir, const QString &srcLocation,
                         const QStringList &extraOptions = {}) final;
    bool synchronousPush(const Utils::FilePath &workingDir, const QString &dstLocation,
                         const QStringList &extraOptions = {}) final;
    void commit(const Utils::FilePath &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile, const QStringList &extraOptions = {}) final;
    void annotate(const Utils::FilePath &workingDir, const QString &file,
                  int lineNumber = -1, const QString &revision = {},
                  const QStringList &extraOptions = {}, int firstLine = -1) final;
    void log(const Utils::FilePath &workingDir, const QStringList &files = {},
             const QStringList &extraOptions = {}, bool enableAnnotationContextMenu = false,
             const std::function<void(Utils::CommandLine &)> &addAuthOptions = {}) final;
    void logCurrentFile(const Utils::FilePath &workingDir, const QStringList &files = {},
                        const QStringList &extraOptions = {},
                        bool enableAnnotationContextMenu = false,
                        const std::function<void(Utils::CommandLine &)> &addAuthOptions = {});
    void revertFile(const Utils::FilePath &workingDir, const QString &file,
                    const QString &revision = {}, const QStringList &extraOptions = {}) final;
    void revertAll(const Utils::FilePath &workingDir, const QString &revision = {},
                   const QStringList &extraOptions = {}) final;
    bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const;
    Utils::FilePath findTopLevelForFile(const Utils::FilePath &file) const final;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const;
    unsigned int binaryVersion() const;
    QString binaryVersionString() const;
    SupportedFeatures supportedFeatures() const;
    void view(const Utils::FilePath &source, const QString &id, const QStringList &extraOptions = {}) final;

private:
    static QList<BranchInfo> branchListFromOutput(const QString &output,
                                                  const BranchInfo::BranchFlags defaultFlags = {});
    static QStringList parseRevisionCommentLine(const QString &commentLine);

    QString sanitizeFossilOutput(const QString &output) const;
    QString vcsCommandString(VcsCommandTag cmd) const final;
    Utils::Id vcsEditorKind(VcsCommandTag cmd) const final;
    QStringList revisionSpec(const QString &revision) const final;
    StatusItem parseStatusLine(const QString &line) const final;
    VcsBase::VcsBaseEditorConfig *createAnnotateEditor(VcsBase::VcsBaseEditorWidget *editor);
    VcsBase::VcsBaseEditorConfig *createLogCurrentFileEditor(VcsBase::VcsBaseEditorWidget *editor);
    VcsBase::VcsBaseEditorConfig *createLogEditor(VcsBase::VcsBaseEditorWidget *editor);

    friend class FossilPluginPrivate;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FossilClient::SupportedFeatures)

} // namespace Internal
} // namespace Fossil
