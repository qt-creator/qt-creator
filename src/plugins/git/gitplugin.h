// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/iversioncontrol.h>

#include <functional>

#include <vcsbase/vcsbaseplugin.h>

namespace VcsBase { class VcsBasePluginState; }

namespace Git::Internal {

Core::IVersionControl *versionControl();
const VcsBase::VcsBasePluginState &currentState();

QString msgRepositoryLabel(const Utils::FilePath &repository);
QString invalidBranchAndRemoteNamePattern();
bool isCommitEditorOpen();

void emitFilesChanged(const QStringList &);
void emitRepositoryChanged(const Utils::FilePath &);
void startRebaseFromCommit(const Utils::FilePath &workingDirectory, const QString &commit);
void manageRemotes();
void initRepository();
void startCommit();
void updateCurrentBranch();
void updateBranches(const Utils::FilePath &repository);
void gerritPush(const Utils::FilePath &topLevel);

} // Git::Internal
