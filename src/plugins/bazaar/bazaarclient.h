/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef BAZAARCLIENT_H
#define BAZAARCLIENT_H

#include "bazaarsettings.h"
#include "branchinfo.h"
#include <vcsbase/vcsbaseclient.h>

namespace Bazaar {
namespace Internal {

class BazaarSettings;

class BazaarClient : public VCSBase::VCSBaseClient
{
    Q_OBJECT
public:
    BazaarClient(BazaarSettings *settings);

    BazaarSettings *settings() const;

    bool synchronousSetUserId();
    BranchInfo synchronousBranchQuery(const QString &repositoryRoot) const;
    QString findTopLevelForFile(const QFileInfo &file) const;

protected:
    QString vcsEditorKind(VCSCommand cmd) const;

    QStringList cloneArguments(const QString &srcLocation,
                               const QString &dstLocation,
                               const QStringList &extraOptions) const;
    QStringList pullArguments(const QString &srcLocation,
                              const QStringList &extraOptions) const;
    QStringList pushArguments(const QString &dstLocation,
                              const QStringList &extraOptions) const;
    QStringList commitArguments(const QStringList &files,
                                const QString &commitMessageFile,
                                const QStringList &extraOptions) const;
    QStringList importArguments(const QStringList &files) const;
    QStringList updateArguments(const QString &revision) const;
    QStringList revertArguments(const QString &file, const QString &revision) const;
    QStringList revertAllArguments(const QString &revision) const;
    QStringList annotateArguments(const QString &file,
                                  const QString &revision, int lineNumber) const;
    QStringList diffArguments(const QStringList &files,
                              const QStringList &extraOptions) const;
    VCSBase::VCSBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                            const QStringList &files,
                                                            const QStringList &extraOptions);
    QStringList logArguments(const QStringList &files,
                             const QStringList &extraOptions) const;
    QStringList statusArguments(const QString &file) const;
    QStringList viewArguments(const QString &revision) const;

    QPair<QString, QString> parseStatusLine(const QString &line) const;
private:
    friend class CloneWizard;
};

} //namespace Internal
} // namespace Bazaar

#endif // BAZAARCLIENT_H
