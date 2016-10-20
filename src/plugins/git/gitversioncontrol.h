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

#include <coreplugin/iversioncontrol.h>

namespace Git {
namespace Internal {

class GitClient;

// Just a proxy for GitPlugin
class GitVersionControl : public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit GitVersionControl(GitClient *client);

    QString displayName() const final;
    Core::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FileName &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const QString &fileName) final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    bool vcsAnnotate(const QString &file, int line) final;
    QString vcsTopic(const QString &directory) final;

    Core::ShellCommand *createInitialCheckoutCommand(const QString &url,
                                                     const Utils::FileName &baseDirectory,
                                                     const QString &localName,
                                                     const QStringList &extraArgs) final;

    QStringList additionalToolsPath() const final;

    void emitFilesChanged(const QStringList &);
    void emitRepositoryChanged(const QString &);

private:
    GitClient *m_client;
};

} // Internal
} // Git
