/**************************************************************************
**
** Copyright (c) 2013 Brian McGillion
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MERCURIALCLIENT_H
#define MERCURIALCLIENT_H

#include "mercurialsettings.h"
#include <vcsbase/vcsbaseclient.h>

namespace Mercurial {
namespace Internal {
struct MercurialDiffParameters;

class MercurialClient : public VcsBase::VcsBaseClient
{
    Q_OBJECT
public:
    MercurialClient(MercurialSettings *settings);

    MercurialSettings *settings() const;

    bool synchronousClone(const QString &workingDir,
                          const QString &srcLocation,
                          const QString &dstLocation,
                          const QStringList &extraOptions = QStringList());
    bool manifestSync(const QString &repository, const QString &filename);
    QString branchQuerySync(const QString &repositoryRoot);
    QStringList parentRevisionsSync(const QString &workingDirectory,
                             const QString &file /* = QString() */,
                             const QString &revision);
    QString shortDescriptionSync(const QString &workingDirectory, const QString &revision,
                              const QString &format /* = QString() */);
    QString shortDescriptionSync(const QString &workingDirectory, const QString &revision);
    void incoming(const QString &repositoryRoot, const QString &repository = QString());
    void outgoing(const QString &repositoryRoot);
    QString vcsGetRepositoryURL(const QString &directory);

    void annotate(const QString &workingDir, const QString &file,
                  const QString revision = QString(), int lineNumber = -1,
                  const QStringList &extraOptions = QStringList());
    void commit(const QString &repositoryRoot, const QStringList &files,
                const QString &commitMessageFile,
                const QStringList &extraOptions = QStringList());
    void diff(const QString &workingDir, const QStringList &files = QStringList(),
              const QStringList &extraOptions = QStringList());
    void import(const QString &repositoryRoot, const QStringList &files,
                const QStringList &extraOptions = QStringList());
    void revertAll(const QString &workingDir, const QString &revision = QString(),
                   const QStringList &extraOptions = QStringList());
    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = QStringList());

public:
    QString findTopLevelForFile(const QFileInfo &file) const;

protected:
    QString vcsEditorKind(VcsCommand cmd) const;
    QStringList revisionSpec(const QString &revision) const;
    VcsBase::VcsBaseEditorParameterWidget *createDiffEditor(const QString &workingDir,
                                                            const QStringList &files,
                                                            const QStringList &extraOptions);
    StatusItem parseStatusLine(const QString &line) const;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
