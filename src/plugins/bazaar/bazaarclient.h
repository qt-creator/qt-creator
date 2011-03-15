/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef BAZAARCLIENT_H
#define BAZAARCLIENT_H

#include "branchinfo.h"
#include <vcsbase/vcsbaseclient.h>

namespace Bazaar {
namespace Internal {

class BazaarClient : public VCSBase::VCSBaseClient
{
public:
    enum ExtraOptionId
    {
        // Clone
        UseExistingDirCloneOptionId,
        StackedCloneOptionId,
        StandAloneCloneOptionId,
        BindCloneOptionId,
        SwitchCloneOptionId,
        HardLinkCloneOptionId,
        NoTreeCloneOptionId,
        RevisionCloneOptionId,
        // Commit
        AuthorCommitOptionId,
        FixesCommitOptionId,
        LocalCommitOptionId,
        // Pull or push (common options)
        RememberPullOrPushOptionId,
        OverwritePullOrPushOptionId,
        RevisionPullOrPushOptionId,
        // Pull only
        LocalPullOptionId,
        // Push only
        UseExistingDirPushOptionId,
        CreatePrefixPushOptionId
    };

    BazaarClient(const VCSBase::VCSBaseClientSettings &settings);

    bool synchronousSetUserId();
    BranchInfo synchronousBranchQuery(const QString &repositoryRoot) const;
    virtual QString findTopLevelForFile(const QFileInfo &file) const;

protected:
    virtual QString vcsEditorKind(VCSCommand cmd) const;

    virtual QStringList cloneArguments(const QString &srcLocation,
                                       const QString &dstLocation,
                                       const ExtraCommandOptions &extraOptions) const;
    virtual QStringList pullArguments(const QString &srcLocation,
                                      const ExtraCommandOptions &extraOptions) const;
    virtual QStringList pushArguments(const QString &dstLocation,
                                      const ExtraCommandOptions &extraOptions) const;
    virtual QStringList commitArguments(const QStringList &files,
                                        const QString &commitMessageFile,
                                        const ExtraCommandOptions &extraOptions) const;
    virtual QStringList importArguments(const QStringList &files) const;
    virtual QStringList updateArguments(const QString &revision) const;
    virtual QStringList revertArguments(const QString &file, const QString &revision) const;
    virtual QStringList revertAllArguments(const QString &revision) const;
    virtual QStringList annotateArguments(const QString &file,
                                          const QString &revision, int lineNumber) const;
    virtual QStringList diffArguments(const QStringList &files) const;
    virtual QStringList logArguments(const QStringList &files) const;
    virtual QStringList statusArguments(const QString &file) const;
    virtual QStringList viewArguments(const QString &revision) const;

    virtual QPair<QString, QString> parseStatusLine(const QString &line) const;
private:
    QStringList commonPullOrPushArguments(const ExtraCommandOptions &extraOptions) const;
    friend class CloneWizard;
};

} //namespace Internal
} // namespace Bazaar

#endif // BAZAARCLIENT_H
