// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "git_global.h"

#include <coreplugin/iversioncontrol.h>

#include <extensionsystem/iplugin.h>

#include <functional>

#include <vcsbase/vcsbaseplugin.h>

namespace VcsBase { class VcsBasePluginState; }

namespace Git::Internal {

class GITSHARED_EXPORT GitPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Git.json")

public:
    ~GitPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;

    QObject *remoteCommand(const QStringList &options, const QString &workingDirectory,
                           const QStringList &args) final;

    static Core::IVersionControl *versionControl();
    static const VcsBase::VcsBasePluginState &currentState();

    static QString msgRepositoryLabel(const Utils::FilePath &repository);
    static QString invalidBranchAndRemoteNamePattern();
    static bool isCommitEditorOpen();

    static void emitFilesChanged(const QStringList &);
    static void emitRepositoryChanged(const Utils::FilePath &);
    static void startRebaseFromCommit(const Utils::FilePath &workingDirectory, const QString &commit);
    static void manageRemotes();
    static void initRepository();
    static void startCommit();
    static void updateCurrentBranch();
    static void updateBranches(const Utils::FilePath &repository);
    static void gerritPush(const Utils::FilePath &topLevel);

#ifdef WITH_TESTS
private slots:
    void testStatusParsing_data();
    void testStatusParsing();
    void testDiffFileResolving_data();
    void testDiffFileResolving();
    void testLogResolving();
    void testGitRemote_data();
    void testGitRemote();
#endif
};

} // Git::Internal
