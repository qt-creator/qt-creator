/****************************************************************************
**
** Copyright (C) 2016 Brian McGillion
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
    MercurialClient();

    bool synchronousClone(const QString &workingDir,
                          const QString &srcLocation,
                          const QString &dstLocation,
                          const QStringList &extraOptions = QStringList());
    bool synchronousPull(const QString &workingDir,
                         const QString &srcLocation,
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
    bool managesFile(const QString &workingDirectory, const QString &fileName) const;

    void annotate(const QString &workingDir, const QString &file,
                  const QString &revision = QString(), int lineNumber = -1,
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

public slots:
    void view(const QString &source, const QString &id,
              const QStringList &extraOptions = QStringList());

public:
    QString findTopLevelForFile(const QFileInfo &file) const;

protected:
    Core::Id vcsEditorKind(VcsCommandTag cmd) const;
    QStringList revisionSpec(const QString &revision) const;
    StatusItem parseStatusLine(const QString &line) const;

signals:
    void needUpdate();
    void needMerge();

private:
    void parsePullOutput(const QString &output);
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALCLIENT_H
