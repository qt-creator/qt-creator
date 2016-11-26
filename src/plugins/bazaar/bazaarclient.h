/****************************************************************************
**
** Copyright (C) 2016 Hugues Delorme
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

#include "bazaarsettings.h"
#include "branchinfo.h"
#include <vcsbase/vcsbaseclient.h>

namespace Bazaar {
namespace Internal {

class BazaarSettings;
class BazaarControl;

class BazaarClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT

public:
    BazaarClient();

    bool synchronousSetUserId();
    BranchInfo synchronousBranchQuery(const QString &repositoryRoot) const;
    bool synchronousUncommit(const QString &workingDir,
                             const QString& revision = QString(),
                             const QStringList &extraOptions = QStringList());
    void commit(const QString &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile, const QStringList &extraOptions = QStringList());
    VcsBase::VcsBaseEditorWidget *annotate(
            const QString &workingDir, const QString &file, const QString &revision = QString(),
            int lineNumber = -1, const QStringList &extraOptions = QStringList());
    bool isVcsDirectory(const Utils::FileName &fileName) const;
    QString findTopLevelForFile(const QFileInfo &file) const;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;
    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = QStringList());

protected:
    Core::Id vcsEditorKind(VcsCommandTag cmd) const;
    QString vcsCommandString(VcsCommandTag cmd) const;
    Utils::ExitCodeInterpreter exitCodeInterpreter(VcsCommandTag cmd) const;
    QStringList revisionSpec(const QString &revision) const;
    StatusItem parseStatusLine(const QString &line) const;

private:
    friend class CloneWizard;
    friend class BazaarControl;
};

} // namespace Internal
} // namespace Bazaar
