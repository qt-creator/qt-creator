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

namespace Perforce {
namespace Internal {
class PerforcePlugin;

// Just a proxy for PerforcePlugin
class PerforceVersionControl : public Core::IVersionControl
{
    Q_OBJECT
public:
    explicit PerforceVersionControl(PerforcePlugin *plugin);

    QString displayName() const final;
    Core::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FileName &fileName) const final;
    bool managesDirectory(const QString &directory, QString *topLevel = 0) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const QString &fileName) const final;
    bool vcsOpen(const QString &fileName) final;
    SettingsFlags settingsFlags() const final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;
    bool vcsAnnotate(const QString &file, int line) final;
    QString vcsOpenText() const final;
    QString vcsMakeWritableText() const final;

    void emitRepositoryChanged(const QString &s);
    void emitFilesChanged(const QStringList &l);
    void emitConfigurationChanged();

private:
    PerforcePlugin *m_plugin;
};

} // namespace Internal
} // namespace Perforce
