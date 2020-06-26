/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "gitsettings.h"

#include <coreplugin/iversioncontrol.h>

#include <extensionsystem/iplugin.h>

#include <functional>

#include <vcsbase/vcsbaseplugin.h>

namespace VcsBase { class VcsBasePluginState; }

namespace Git {
namespace Internal {

class GitClient;

class GitPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Git.json")

public:
    ~GitPlugin() final;

    bool initialize(const QStringList &arguments, QString *errorMessage) final;
    void extensionsInitialized() final;

    QObject *remoteCommand(const QStringList &options, const QString &workingDirectory,
                           const QStringList &args) final;

    static GitClient *client();
    static Core::IVersionControl *versionControl();
    static const GitSettings &settings();
    static const VcsBase::VcsBasePluginState &currentState();

    static QString msgRepositoryLabel(const QString &repository);
    static QString invalidBranchAndRemoteNamePattern();
    static bool isCommitEditorOpen();

    static void emitFilesChanged(const QStringList &);
    static void emitRepositoryChanged(const QString &);
    static void startRebaseFromCommit(const QString &workingDirectory, const QString &commit);
    static void manageRemotes();
    static void initRepository();
    static void startCommit();
    static void updateCurrentBranch();
    static void updateBranches(const QString &repository);
    static void gerritPush(const QString &topLevel);

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

} // namespace Internal
} // namespace Git
